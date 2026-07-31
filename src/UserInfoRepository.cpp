#include "BudgetBitesLib/UserInfoRepository.h"

#include "BudgetBitesLib/CsvFile.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace {

using Row = std::vector<std::string>;

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::optional<int> integer(const std::string& value) {
    try {
        std::size_t used = 0;
        const int result = std::stoi(value, &used);
        return used == value.size() ? std::optional<int>(result) : std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> real(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }
    try {
        std::size_t used = 0;
        const double result = std::stod(value, &used);
        return used == value.size() ? std::optional<double>(result) : std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::string decimal(double value) {
    std::ostringstream output;
    output << std::setprecision(15) << value;
    return output.str();
}

std::vector<int> uniqueIds(const std::vector<int>& values) {
    std::vector<int> result;
    std::unordered_set<int> seen;
    for (const int value : values) {
        if (seen.insert(value).second) {
            result.push_back(value);
        }
    }
    return result;
}

} // namespace

UserInfoRepository::UserInfoRepository(const std::filesystem::path& storageDirectory)
    : storageDirectory_(storageDirectory) {
    load();
}

bool UserInfoRepository::load() {
    usersById_.clear();
    dietaryTagIdsByUser_.clear();
    allergenIdsByUser_.clear();
    pantryItemsByUser_.clear();
    lastError_.clear();

    const auto readOptional = [&](const std::string& filename, const std::vector<std::string>& headers, CsvFile::Table& table) {
        const auto path = storageDirectory_ / filename;
        // Missing files are normal before the first local save.
        if (!std::filesystem::exists(path)) {
            table.headers = headers;
            return true;
        }
        std::string error;
        if (!CsvFile::read(path, table, error)) {
            return fail(error);
        }
        return table.headers == headers ? true : fail(filename + " headers do not match the user-data schema.");
    };

    CsvFile::Table users;
    if (!readOptional("user_info.csv", {"user_id", "username", "password_hash", "password_salt", "profile_image_path", "weekly_budget"}, users)) {
        return false;
    }
    for (const Row& row : users.rows) {
        const auto userId = integer(row[0]);
        const auto budget = real(row[5]);
        if (!userId || !budget || *budget < 0.0 || row[1].empty() || !usersById_.emplace(*userId, UserInfo{*userId, row[1], row[2], row[3], row[4], *budget}).second) {
            return fail("user_info.csv contains an invalid or duplicate record.");
        }
    }

    const auto loadIdRelations = [&](const std::string& filename, const std::string& valueHeader, std::unordered_map<int, std::vector<int>>& target) {
        CsvFile::Table table;
        if (!readOptional(filename, {"user_id", valueHeader}, table)) {
            return false;
        }
        for (const Row& row : table.rows) {
            const auto userId = integer(row[0]);
            const auto valueId = integer(row[1]);
            if (!userId || !valueId || !hasUser(*userId)) {
                return fail(filename + " contains an invalid linked record.");
            }
            target[*userId].push_back(*valueId);
        }
        for (auto& entry : target) {
            entry.second = uniqueIds(entry.second);
        }
        return true;
    };
    if (!loadIdRelations("user_dietary_tags.csv", "dietary_tag_id", dietaryTagIdsByUser_) ||
        !loadIdRelations("user_allergens.csv", "allergen_id", allergenIdsByUser_)) {
        return false;
    }

    CsvFile::Table pantryItems;
    if (!readOptional("user_pantry_items.csv", {"user_id", "ingredient_id", "available_grams"}, pantryItems)) {
        return false;
    }
    for (const Row& row : pantryItems.rows) {
        const auto userId = integer(row[0]);
        const auto ingredientId = integer(row[1]);
        const auto grams = real(row[2]);
        if (!userId || !ingredientId || (row[2].size() && (!grams || *grams < 0.0)) || !hasUser(*userId)) {
            return fail("user_pantry_items.csv contains an invalid linked record.");
        }
        pantryItemsByUser_[*userId].push_back({*ingredientId, grams});
    }
    return true;
}

bool UserInfoRepository::save() {
    std::vector<int> userIds;
    userIds.reserve(usersById_.size());
    for (const auto& entry : usersById_) {
        userIds.push_back(entry.first);
    }
    // Stable row order keeps local files easy to review.
    std::sort(userIds.begin(), userIds.end());

    std::vector<Row> users;
    std::vector<Row> dietaryTags;
    std::vector<Row> allergens;
    std::vector<Row> pantryItems;
    for (const int userId : userIds) {
        const UserInfo& user = usersById_.at(userId);
        users.push_back({std::to_string(user.id), user.username, user.passwordHash, user.passwordSalt, user.profileImagePath, decimal(user.weeklyBudget)});
        for (const int tagId : getDietaryTagIds(userId)) {
            dietaryTags.push_back({std::to_string(userId), std::to_string(tagId)});
        }
        for (const int allergenId : getAllergenIds(userId)) {
            allergens.push_back({std::to_string(userId), std::to_string(allergenId)});
        }
        for (const PantryItem& item : getPantryItems(userId)) {
            pantryItems.push_back({std::to_string(userId), std::to_string(item.ingredientId), item.availableGrams ? decimal(*item.availableGrams) : ""});
        }
    }

    std::string error;
    if (!CsvFile::write(storageDirectory_ / "user_info.csv", {"user_id", "username", "password_hash", "password_salt", "profile_image_path", "weekly_budget"}, users, error) ||
        !CsvFile::write(storageDirectory_ / "user_dietary_tags.csv", {"user_id", "dietary_tag_id"}, dietaryTags, error) ||
        !CsvFile::write(storageDirectory_ / "user_allergens.csv", {"user_id", "allergen_id"}, allergens, error) ||
        !CsvFile::write(storageDirectory_ / "user_pantry_items.csv", {"user_id", "ingredient_id", "available_grams"}, pantryItems, error)) {
        return fail(error);
    }
    return true;
}

const std::string& UserInfoRepository::lastError() const noexcept {
    return lastError_;
}

std::filesystem::path UserInfoRepository::defaultStorageDirectory() {
    return "data/local";
}

std::optional<UserInfo> UserInfoRepository::createUser(
    const std::string& username,
    const std::string& passwordHash,
    const std::string& passwordSalt
) {
    if (username.empty() || getUserByUsername(username)) {
        lastError_ = "Username is empty or already exists.";
        return std::nullopt;
    }
    int nextId = 1;
    for (const auto& entry : usersById_) {
        nextId = std::max(nextId, entry.first + 1);
    }
    UserInfo user{nextId, username, passwordHash, passwordSalt, "", 0.0};
    usersById_.emplace(user.id, user);
    return user;
}

std::optional<UserInfo> UserInfoRepository::getUserById(int userId) const {
    const auto found = usersById_.find(userId);
    return found == usersById_.end() ? std::nullopt : std::optional<UserInfo>(found->second);
}

std::optional<UserInfo> UserInfoRepository::getUserByUsername(const std::string& username) const {
    const auto target = lower(username);
    for (const auto& entry : usersById_) {
        if (lower(entry.second.username) == target) {
            return entry.second;
        }
    }
    return std::nullopt;
}

bool UserInfoRepository::updateProfileImagePath(int userId, const std::string& profileImagePath) {
    const auto found = usersById_.find(userId);
    if (found == usersById_.end()) {
        return fail("User does not exist.");
    }
    found->second.profileImagePath = profileImagePath;
    return true;
}

bool UserInfoRepository::updateWeeklyBudget(int userId, double weeklyBudget) {
    const auto found = usersById_.find(userId);
    if (found == usersById_.end() || weeklyBudget < 0.0) {
        return fail("User does not exist or budget is negative.");
    }
    found->second.weeklyBudget = weeklyBudget;
    return true;
}

bool UserInfoRepository::replaceDietaryTagIds(int userId, const std::vector<int>& dietaryTagIds) {
    if (!hasUser(userId)) {
        return fail("User does not exist.");
    }
    dietaryTagIdsByUser_[userId] = uniqueIds(dietaryTagIds);
    return true;
}

bool UserInfoRepository::replaceAllergenIds(int userId, const std::vector<int>& allergenIds) {
    if (!hasUser(userId)) {
        return fail("User does not exist.");
    }
    allergenIdsByUser_[userId] = uniqueIds(allergenIds);
    return true;
}

bool UserInfoRepository::replacePantryItems(int userId, const std::vector<PantryItem>& pantryItems) {
    if (!hasUser(userId) || std::any_of(pantryItems.begin(), pantryItems.end(), [](const PantryItem& item) {
        return item.ingredientId <= 0 || (item.availableGrams && *item.availableGrams < 0.0);
    })) {
        return fail("User does not exist or pantry item is invalid.");
    }
    pantryItemsByUser_[userId] = pantryItems;
    return true;
}

std::vector<int> UserInfoRepository::getDietaryTagIds(int userId) const {
    const auto found = dietaryTagIdsByUser_.find(userId);
    return found == dietaryTagIdsByUser_.end() ? std::vector<int>{} : found->second;
}

std::vector<int> UserInfoRepository::getAllergenIds(int userId) const {
    const auto found = allergenIdsByUser_.find(userId);
    return found == allergenIdsByUser_.end() ? std::vector<int>{} : found->second;
}

std::vector<PantryItem> UserInfoRepository::getPantryItems(int userId) const {
    const auto found = pantryItemsByUser_.find(userId);
    return found == pantryItemsByUser_.end() ? std::vector<PantryItem>{} : found->second;
}

bool UserInfoRepository::fail(const std::string& message) {
    lastError_ = message;
    return false;
}

bool UserInfoRepository::hasUser(int userId) const {
    return usersById_.count(userId) != 0;
}
