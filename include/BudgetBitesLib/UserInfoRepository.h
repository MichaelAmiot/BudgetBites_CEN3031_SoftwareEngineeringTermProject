#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct UserInfo {
    int id;
    std::string username;
    std::string passwordHash;
    std::string passwordSalt;
    std::string profileImagePath;
    double weeklyBudget = 0.0;
};

struct PantryItem {
    int ingredientId;
    // Empty grams means the user has the item but did not measure it.
    std::optional<double> availableGrams;
};

// Stores user-only data in local CSV files, separate from the shared catalog.
class UserInfoRepository {
public:
    explicit UserInfoRepository(const std::filesystem::path& storageDirectory = defaultStorageDirectory());

    bool load();
    bool save();
    const std::string& lastError() const noexcept;
    static std::filesystem::path defaultStorageDirectory();

    std::optional<UserInfo> createUser(
        const std::string& username,
        const std::string& passwordHash,
        const std::string& passwordSalt
    );
    std::optional<UserInfo> getUserById(int userId) const;
    std::optional<UserInfo> getUserByUsername(const std::string& username) const;
    bool updateProfileImagePath(int userId, const std::string& profileImagePath);
    bool updateWeeklyBudget(int userId, double weeklyBudget);

    bool replaceDietaryTagIds(int userId, const std::vector<int>& dietaryTagIds);
    bool replaceAllergenIds(int userId, const std::vector<int>& allergenIds);
    bool replacePantryItems(int userId, const std::vector<PantryItem>& pantryItems);
    std::vector<int> getDietaryTagIds(int userId) const;
    std::vector<int> getAllergenIds(int userId) const;
    std::vector<PantryItem> getPantryItems(int userId) const;

private:
    bool fail(const std::string& message);
    bool hasUser(int userId) const;

    std::filesystem::path storageDirectory_;
    std::unordered_map<int, UserInfo> usersById_;
    // User choices keep catalog IDs, so catalog names can change safely.
    std::unordered_map<int, std::vector<int>> dietaryTagIdsByUser_;
    std::unordered_map<int, std::vector<int>> allergenIdsByUser_;
    std::unordered_map<int, std::vector<PantryItem>> pantryItemsByUser_;
    std::string lastError_;
};
