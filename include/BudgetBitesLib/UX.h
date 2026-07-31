#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "UserInfoRepository.h"

class UX {
public:
    explicit UX(const std::filesystem::path& storageDirectory = UserInfoRepository::defaultStorageDirectory());

    bool registerUser(const std::string& username, const std::string& password);
    bool signIn(const std::string& username, const std::string& password);
    void signOut();

    bool isSignedIn() const;
    std::optional<std::string> currentUser() const;
    std::optional<UserInfo> currentUserInfo() const;
    bool uploadProfileImage(const std::string& username, const std::string& sourceImagePath);
    std::optional<std::string> getProfileImagePath(const std::string& username) const;

    // These calls save the signed-in user's changes right away.
    bool updateCurrentWeeklyBudget(double weeklyBudget);
    bool replaceCurrentDietaryTagIds(const std::vector<int>& dietaryTagIds);
    bool replaceCurrentAllergenIds(const std::vector<int>& allergenIds);
    bool replaceCurrentPantryItems(const std::vector<PantryItem>& pantryItems);
    std::vector<int> getCurrentDietaryTagIds() const;
    std::vector<int> getCurrentAllergenIds() const;
    std::vector<PantryItem> getCurrentPantryItems() const;

    bool saveUserData();
    bool reloadUserData();
    static bool isPasswordStrong(const std::string& password);

private:
    UserInfoRepository repository;
    std::optional<int> currentUserId;
};
