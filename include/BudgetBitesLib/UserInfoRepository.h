#pragma once

#include "BudgetBitesLib/Ingredients.h"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct UserExtraInfo {
    std::string username;
    double weeklyBudget = 0.0;
};

// Stores user-only data in local CSV files, separate from the shared catalog.
class UserInfoRepository {
public:
    explicit UserInfoRepository(const std::filesystem::path& storageDirectory = defaultStorageDirectory());

    bool load();
    bool save();
    const std::string& lastError() const noexcept;
    static std::filesystem::path defaultStorageDirectory();

    // Adds a default supplemental record if this username does not have one yet.
    bool ensureUser(const std::string& username);
    std::optional<UserExtraInfo> getUserByUsername(const std::string& username) const;
    bool updateWeeklyBudget(const std::string& username, double weeklyBudget);

    bool replaceDietaryTagIds(const std::string& username, const std::vector<int>& dietaryTagIds);
    bool replaceAllergenIds(const std::string& username, const std::vector<int>& allergenIds);
    bool replacePantryItems(const std::string& username, const std::vector<PantryItem>& pantryItems);
    std::vector<int> getDietaryTagIds(const std::string& username) const;
    std::vector<int> getAllergenIds(const std::string& username) const;
    std::vector<PantryItem> getPantryItems(const std::string& username) const;

private:
    bool fail(const std::string& message);
    bool hasUser(const std::string& username) const;

    std::filesystem::path storageDirectory_;
    // Lowercase usernames are internal keys; the original spelling is kept for saving.
    std::unordered_map<std::string, UserExtraInfo> usersByUsername_;
    // User choices keep catalog IDs, so catalog names can change safely.
    std::unordered_map<std::string, std::vector<int>> dietaryTagIdsByUser_;
    std::unordered_map<std::string, std::vector<int>> allergenIdsByUser_;
    std::unordered_map<std::string, std::vector<PantryItem>> pantryItemsByUser_;
    std::string lastError_;
};
