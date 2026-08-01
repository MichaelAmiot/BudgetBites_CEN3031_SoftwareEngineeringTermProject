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
    for (char& character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

std::optional<int> integer(const std::string& value) {
    try {
        std::size_t used = 0;
        const int result = std::stoi(value, &used);
        if (used != value.size()) {
            return std::nullopt;
        }
        return result;
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
        if (used != value.size()) {
            return std::nullopt;
        }
        return result;
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
    for (int value : values) {
        if (!seen.count(value)) {
            seen.insert(value);
            result.push_back(value);
        }
    }
    return result;
}

bool readOptionalTable(
    const std::filesystem::path& storageDirectory,
    const std::string& filename,
    const std::vector<std::string>& headers,
    CsvFile::Table& table,
    std::string& error
) {
    const std::filesystem::path path = storageDirectory / filename;
    if (!std::filesystem::exists(path)) {
        table.headers = headers;
        return true;
    }
    if (!CsvFile::read(path, table, error)) {
        return false;
    }
    if (table.headers != headers) {
        error = filename + " headers do not match the user-data schema.";
        return false;
    }
    return true;
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
    std::string error;

    CsvFile::Table users;
    if (!readOptionalTable(
            storageDirectory_,
            "user_info.csv",
            {"username", "weekly_budget"},
            users,
            error)) {
        return fail(error);
    }
    for (const Row& row : users.rows) {
        const auto budget = real(row[1]);
        if (row[0].empty() || !budget || *budget < 0.0) {
            return fail("user_info.csv contains an invalid or duplicate record.");
        }

        const std::string usernameKey = lower(row[0]);
        if (usersByUsername_.count(usernameKey)) {
            return fail("user_info.csv contains an invalid or duplicate record.");
        }
        usersByUsername_[usernameKey] = UserExtraInfo{row[0], *budget};
    }

    CsvFile::Table dietaryTags;
    if (!readOptionalTable(
            storageDirectory_,
            "user_dietary_tags.csv",
            {"username", "dietary_tag_id"},
            dietaryTags,
            error)) {
        return fail(error);
    }
    for (const Row& row : dietaryTags.rows) {
        const auto tagId = integer(row[1]);
        if (row[0].empty() || !tagId || !hasUser(row[0])) {
            return fail("user_dietary_tags.csv contains an invalid linked record.");
        }
        dietaryTagIdsByUser_[lower(row[0])].push_back(*tagId);
    }
    for (auto& entry : dietaryTagIdsByUser_) {
        entry.second = uniqueIds(entry.second);
    }

    CsvFile::Table allergens;
    if (!readOptionalTable(
            storageDirectory_,
            "user_allergens.csv",
            {"username", "allergen_id"},
            allergens,
            error)) {
        return fail(error);
    }
    for (const Row& row : allergens.rows) {
        const auto allergenId = integer(row[1]);
        if (row[0].empty() || !allergenId || !hasUser(row[0])) {
            return fail("user_allergens.csv contains an invalid linked record.");
        }
        allergenIdsByUser_[lower(row[0])].push_back(*allergenId);
    }
    for (auto& entry : allergenIdsByUser_) {
        entry.second = uniqueIds(entry.second);
    }

    CsvFile::Table pantryItems;
    if (!readOptionalTable(
            storageDirectory_,
            "user_pantry_items.csv",
            {"username", "ingredient_id", "available_grams"},
            pantryItems,
            error)) {
        return fail(error);
    }
    for (const Row& row : pantryItems.rows) {
        const auto ingredientId = integer(row[1]);
        const auto grams = real(row[2]);
        if (row[0].empty() || !ingredientId || !hasUser(row[0])) {
            return fail("user_pantry_items.csv contains an invalid linked record.");
        }
        if (!row[2].empty() && (!grams || *grams < 0.0)) {
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
    std::sort(usernames.begin(), usernames.end());

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
            std::string availableGrams;
            if (item.availableGrams) {
                availableGrams = decimal(*item.availableGrams);
            }
            pantryItems.push_back({
                user.username,
                std::to_string(item.ingredientId),
                availableGrams
            });
        }
    }

    std::string error;
    if (!CsvFile::write(
            storageDirectory_ / "user_info.csv",
            {"username", "weekly_budget"},
            users,
            error)) {
        return fail(error);
    }
    if (!CsvFile::write(
            storageDirectory_ / "user_dietary_tags.csv",
            {"username", "dietary_tag_id"},
            dietaryTags,
            error)) {
        return fail(error);
    }
    if (!CsvFile::write(
            storageDirectory_ / "user_allergens.csv",
            {"username", "allergen_id"},
            allergens,
            error)) {
        return fail(error);
    }
    if (!CsvFile::write(
            storageDirectory_ / "user_pantry_items.csv",
            {"username", "ingredient_id", "available_grams"},
            pantryItems,
            error)) {
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
    if (found == usersByUsername_.end()) {
        return std::nullopt;
    }
    return found->second;
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
    if (!hasUser(username)) {
        return fail("User does not exist or pantry item is invalid.");
    }
    for (const PantryItem& item : pantryItems) {
        if (item.ingredientId <= 0 || (item.availableGrams && *item.availableGrams < 0.0)) {
            return fail("User does not exist or pantry item is invalid.");
        }
    }
    pantryItemsByUser_[lower(username)] = pantryItems;
    return true;
}

std::vector<int> UserInfoRepository::getDietaryTagIds(const std::string& username) const {
    const auto found = dietaryTagIdsByUser_.find(lower(username));
    if (found == dietaryTagIdsByUser_.end()) {
        return {};
    }
    return found->second;
}

std::vector<int> UserInfoRepository::getAllergenIds(const std::string& username) const {
    const auto found = allergenIdsByUser_.find(lower(username));
    if (found == allergenIdsByUser_.end()) {
        return {};
    }
    return found->second;
}

std::vector<PantryItem> UserInfoRepository::getPantryItems(const std::string& username) const {
    const auto found = pantryItemsByUser_.find(lower(username));
    if (found == pantryItemsByUser_.end()) {
        return {};
    }
    return found->second;
}

bool UserInfoRepository::fail(const std::string& message) {
    lastError_ = message;
    return false;
}

bool UserInfoRepository::hasUser(const std::string& username) const {
    return usersByUsername_.count(lower(username)) != 0;
}
