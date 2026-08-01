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
    usersByUsername_.clear();
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

    // Load the base username and budget table before its linked ID tables.
    CsvFile::Table users;
    if (!readOptional("user_info.csv", {"username", "weekly_budget"}, users)) {
        return false;
    }
    for (const Row& row : users.rows) {
        const auto budget = real(row[1]);
        if (row[0].empty() || !budget || *budget < 0.0 ||
            !usersByUsername_.emplace(lower(row[0]), UserExtraInfo{row[0], *budget}).second) {
            return fail("user_info.csv contains an invalid or duplicate record.");
        }
    }

    // Dietary preferences and allergens use one row per selected catalog ID.
    const auto loadIdRelations = [&](
        const std::string& filename,
        const std::string& valueHeader,
        std::unordered_map<std::string, std::vector<int>>& target
    ) {
        CsvFile::Table table;
        if (!readOptional(filename, {"username", valueHeader}, table)) {
            return false;
        }
        for (const Row& row : table.rows) {
            const auto valueId = integer(row[1]);
            if (row[0].empty() || !valueId || !hasUser(row[0])) {
                return fail(filename + " contains an invalid linked record.");
            }
            target[lower(row[0])].push_back(*valueId);
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

    // A blank available_grams field means the user owns an unmeasured amount.
    CsvFile::Table pantryItems;
    if (!readOptional("user_pantry_items.csv", {"username", "ingredient_id", "available_grams"}, pantryItems)) {
        return false;
    }
    for (const Row& row : pantryItems.rows) {
        const auto ingredientId = integer(row[1]);
        const auto grams = real(row[2]);
        if (row[0].empty() || !ingredientId ||
            (!row[2].empty() && (!grams || *grams < 0.0)) || !hasUser(row[0])) {
            return fail("user_pantry_items.csv contains an invalid linked record.");
        }
        pantryItemsByUser_[lower(row[0])].push_back({*ingredientId, grams});
    }
    return true;
}

bool UserInfoRepository::save() {
    std::vector<std::string> usernames;
    usernames.reserve(usersByUsername_.size());
    for (const auto& entry : usersByUsername_) {
        usernames.push_back(entry.first);
    }
    // Stable row order keeps local files easy to review.
    std::sort(usernames.begin(), usernames.end());

    // Rebuild all four tables from the current in-memory supplemental data.
    std::vector<Row> users;
    std::vector<Row> dietaryTags;
    std::vector<Row> allergens;
    std::vector<Row> pantryItems;
    for (const std::string& usernameKey : usernames) {
        const UserExtraInfo& user = usersByUsername_.at(usernameKey);
        users.push_back({user.username, decimal(user.weeklyBudget)});
        for (const int tagId : getDietaryTagIds(usernameKey)) {
            dietaryTags.push_back({user.username, std::to_string(tagId)});
        }
        for (const int allergenId : getAllergenIds(usernameKey)) {
            allergens.push_back({user.username, std::to_string(allergenId)});
        }
        for (const PantryItem& item : getPantryItems(usernameKey)) {
            pantryItems.push_back({user.username, std::to_string(item.ingredientId), item.availableGrams ? decimal(*item.availableGrams) : ""});
        }
    }

    std::string error;
    if (!CsvFile::write(storageDirectory_ / "user_info.csv", {"username", "weekly_budget"}, users, error) ||
        !CsvFile::write(storageDirectory_ / "user_dietary_tags.csv", {"username", "dietary_tag_id"}, dietaryTags, error) ||
        !CsvFile::write(storageDirectory_ / "user_allergens.csv", {"username", "allergen_id"}, allergens, error) ||
        !CsvFile::write(storageDirectory_ / "user_pantry_items.csv", {"username", "ingredient_id", "available_grams"}, pantryItems, error)) {
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

bool UserInfoRepository::ensureUser(const std::string& username) {
    if (username.empty()) {
        return fail("Username is empty.");
    }
    const std::string usernameKey = lower(username);
    if (!usersByUsername_.count(usernameKey)) {
        usersByUsername_.emplace(usernameKey, UserExtraInfo{username, 0.0});
    }
    return true;
}

std::optional<UserExtraInfo> UserInfoRepository::getUserByUsername(const std::string& username) const {
    const auto found = usersByUsername_.find(lower(username));
    return found == usersByUsername_.end()
        ? std::nullopt
        : std::optional<UserExtraInfo>(found->second);
}

bool UserInfoRepository::updateWeeklyBudget(const std::string& username, double weeklyBudget) {
    const auto found = usersByUsername_.find(lower(username));
    if (found == usersByUsername_.end() || weeklyBudget < 0.0) {
        return fail("User does not exist or budget is negative.");
    }
    found->second.weeklyBudget = weeklyBudget;
    return true;
}

bool UserInfoRepository::replaceDietaryTagIds(
    const std::string& username,
    const std::vector<int>& dietaryTagIds
) {
    if (!hasUser(username)) {
        return fail("User does not exist.");
    }
    dietaryTagIdsByUser_[lower(username)] = uniqueIds(dietaryTagIds);
    return true;
}

bool UserInfoRepository::replaceAllergenIds(
    const std::string& username,
    const std::vector<int>& allergenIds
) {
    if (!hasUser(username)) {
        return fail("User does not exist.");
    }
    allergenIdsByUser_[lower(username)] = uniqueIds(allergenIds);
    return true;
}

bool UserInfoRepository::replacePantryItems(
    const std::string& username,
    const std::vector<PantryItem>& pantryItems
) {
    if (!hasUser(username) || std::any_of(pantryItems.begin(), pantryItems.end(), [](const PantryItem& item) {
        return item.ingredientId <= 0 || (item.availableGrams && *item.availableGrams < 0.0);
    })) {
        return fail("User does not exist or pantry item is invalid.");
    }
    pantryItemsByUser_[lower(username)] = pantryItems;
    return true;
}

std::vector<int> UserInfoRepository::getDietaryTagIds(const std::string& username) const {
    const auto found = dietaryTagIdsByUser_.find(lower(username));
    return found == dietaryTagIdsByUser_.end() ? std::vector<int>{} : found->second;
}

std::vector<int> UserInfoRepository::getAllergenIds(const std::string& username) const {
    const auto found = allergenIdsByUser_.find(lower(username));
    return found == allergenIdsByUser_.end() ? std::vector<int>{} : found->second;
}

std::vector<PantryItem> UserInfoRepository::getPantryItems(const std::string& username) const {
    const auto found = pantryItemsByUser_.find(lower(username));
    return found == pantryItemsByUser_.end() ? std::vector<PantryItem>{} : found->second;
}

bool UserInfoRepository::fail(const std::string& message) {
    lastError_ = message;
    return false;
}

bool UserInfoRepository::hasUser(const std::string& username) const {
    return usersByUsername_.count(lower(username)) != 0;
}
