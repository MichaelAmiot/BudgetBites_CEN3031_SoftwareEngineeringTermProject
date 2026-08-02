#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "UserRepository.h"
#include "UserInfoRepository.h"

class Account;
class Preferences;
class Ingredients;

// Handles registering, signing in, and profile pictures for
// BudgetBites. Uses PasswordSecurity and ProfileImageStore to do the
// actual hashing/file work, this class just has the rules for how
// accounts are supposed to work.
class UX {
public:
    // The default keeps supplemental data in data/local; tests can provide a temporary folder.
    explicit UX(const std::filesystem::path& userInfoStorageDirectory = UserInfoRepository::defaultStorageDirectory())
        : userInfoRepository(userInfoStorageDirectory) {}

    // fails if username is empty, already taken, or password isn't strong enough
    bool registerUser(const std::string& username, const std::string& password);

    // returns true if the username is already registered (case insensitive).
    // Meant to be checked right after the user types a username, before they
    // even choose a password, so they get an immediate "already taken" warning.
    bool isUsernameTaken(const std::string& username);

    // returns true and starts a session if username/password match
    bool signIn(const std::string& username, const std::string& password);

    void signOut();

    bool isSignedIn() const;

    std::optional<std::string> currentUser() const;

    // only works if username is the person currently signed in
    bool uploadProfileImage(const std::string& username, const std::string& sourceImagePath);

    std::optional<std::string> getProfileImagePath(const std::string& username);

    static bool isPasswordStrong(const std::string& password);

    // Loads the signed-in user's saved budget, catalog IDs, and pantry items.
    bool loadCurrentUserInfo(Account& account, Preferences& preferences, Ingredients& ingredients);

    // Saves the signed-in user's budget, catalog IDs, and pantry items.
    bool saveCurrentUserInfo(
        const Account& account,
        const Preferences& preferences,
        const Ingredients& ingredients
    );

    bool saveToFile(const std::string& filePath) const;
    bool loadFromFile(const std::string& filePath);

private:
    UserRepository repository;
    UserInfoRepository userInfoRepository;
    std::optional<std::string> currentUsername;
};
