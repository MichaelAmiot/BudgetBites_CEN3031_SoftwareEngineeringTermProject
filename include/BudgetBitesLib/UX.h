#pragma once   // prevents double-inclusion

#include <optional>
#include <string>

#include "BudgetBitesLib/UserRepository.h"

// -----------------------------------------------------------------------
// UX
//
// Handles user-facing account features for BudgetBites: registration,
// sign-in, and profile image upload.
//
// This class deliberately does NOT know how passwords are hashed, how
// accounts are persisted, or how images are stored on disk -- those are
// separate concerns, each owned by its own small class/namespace
// (PasswordSecurity, UserRepository, ProfileImageStore). UX only knows
// the *business rules* that tie them together:
//   - a username must be unique before it can be registered
//   - a password must meet the complexity policy before registration
//   - only the signed-in user may upload their own profile image
//
// This split follows two principles directly:
//   - Separation of concerns: "each abstraction in the program should
//     address a separate concern, and all aspects of that concern
//     should be covered there."
//   - Single responsibility: "design classes so that there is only a
//     single reason to change a class." UX only changes when the
//     *account rules* change; it's unaffected if the hashing algorithm,
//     file format, or image folder structure changes.
// -----------------------------------------------------------------------
class UX {
public:
    UX() = default;

    // Registers a new user account.
    // Fails (returns false) if:
    //   - the username is empty
    //   - the username is already taken by another account (comparison
    //     is case-insensitive, so "Alice" and "alice" are treated as the
    //     same username and cannot both be registered)
    //   - the password does not meet the complexity requirements
    //     (see PasswordSecurity::isStrong)
    bool registerUser(const std::string& username, const std::string& password);

    // Attempts to sign in with a username/password pair.
    // On success, marks that user as the current session and returns true.
    // On failure (unknown user or wrong password), returns false and the
    // current session is left unsigned-in.
    bool signIn(const std::string& username, const std::string& password);

    // Signs out the current session, if any.
    void signOut();

    // Returns true if a user is currently signed in.
    bool isSignedIn() const;

    // Returns the username of the currently signed-in user, if any.
    std::optional<std::string> currentUser() const;


    // Uploads a profile image for the given username.
    // Requires that 'username' is the currently signed-in user, so a
    // user cannot upload an image for someone else's account.
    // See ProfileImageStore for the validation/storage rules.
    bool uploadProfileImage(const std::string& username, const std::string& sourceImagePath);

    // Returns the stored profile image path for a user, if one exists.
    std::optional<std::string> getProfileImagePath(const std::string& username) const;

    // Checks whether a candidate password satisfies the complexity
    // policy. Kept here too (delegating to PasswordSecurity) so existing
    // callers don't need to know that responsibility moved.
    static bool isPasswordStrong(const std::string& password);

    // Persists all registered accounts to disk at 'filePath'.
    // Delegates directly to the UserRepository -- UX itself has no idea
    // what the file looks like on disk.
    bool saveToFile(const std::string& filePath) const;

    // Loads accounts previously written by saveToFile. The current
    // signed-in session (if any) is cleared.
    bool loadFromFile(const std::string& filePath);

private:
    UserRepository repository_;
    std::optional<std::string> currentUsername_;
};
