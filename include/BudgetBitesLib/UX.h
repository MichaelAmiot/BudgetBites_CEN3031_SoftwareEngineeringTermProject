#pragma once

#include <iostream>
#include <optional>
#include <string>

#include "UserRepository.h"

// Handles registering, signing in, and profile pictures for
// BudgetBites. Uses PasswordSecurity and ProfileImageStore to do the
// actual hashing/file work, this class just has the rules for how
// accounts are supposed to work.
class UX {
public:
    UX() {}

    // fails if username is empty, already taken, or password isn't strong enough
    bool registerUser(const std::string& username, const std::string& password);

    // returns true and starts a session if username/password match
    bool signIn(const std::string& username, const std::string& password);

    void signOut();

    bool isSignedIn() const;

    std::optional<std::string> currentUser() const;

    void checkUserAndImage(UX& ux) {
        auto user = ux.currentUser();
        if (!user) {
            std::cout << "You must sign in first.\n";
            return;
        }

        auto path = ux.getProfileImagePath(*user);
        if (path) {
            // ...
        }
    }

    // only works if username is the person currently signed in
    bool uploadProfileImage(const std::string& username, const std::string& sourceImagePath);

    std::optional<std::string> getProfileImagePath(const std::string& username);

    static bool isPasswordStrong(const std::string& password);

    bool saveToFile(const std::string& filePath) const;
    bool loadFromFile(const std::string& filePath);

private:
    UserRepository repository;
    std::optional<std::string> currentUsername;
};
