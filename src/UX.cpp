#include "BudgetBitesLib/UX.h"

#include "BudgetBitesLib/PasswordSecurity.h"
#include "BudgetBitesLib/ProfileImageStore.h"

bool UX::registerUser(const std::string& username, const std::string& password) {
    if (username.empty()) {
        return false;
    }
    if (repository_.find(username) != nullptr) {
        return false; // username already taken (case-insensitive)
    }
    if (!PasswordSecurity::isStrong(password)) {
        return false;
    }

    UserAccount account;
    account.username = username;
    account.passwordSalt = PasswordSecurity::generateSalt();
    account.passwordHash = PasswordSecurity::hash(password, account.passwordSalt);

    repository_.add(account);
    return true;
}

bool UX::signIn(const std::string& username, const std::string& password) {
    const UserAccount* account = repository_.find(username);
    if (account == nullptr) {
        // Do the same amount of work whether the user exists or not, so
        // failed logins don't reveal valid usernames via timing.
        PasswordSecurity::hash(password, PasswordSecurity::generateSalt());
        return false;
    }

    if (!PasswordSecurity::verify(password, account->passwordSalt, account->passwordHash)) {
        return false;
    }

    currentUsername_ = username;
    return true;
}

void UX::signOut() {
    currentUsername_.reset();
}

bool UX::isSignedIn() const {
    return currentUsername_.has_value();
}

std::optional<std::string> UX::currentUser() const {
    return currentUsername_;
}

bool UX::uploadProfileImage(const std::string& username, const std::string& sourceImagePath) {
    // Only the signed-in user may upload their own profile image.
    if (!currentUsername_.has_value() || *currentUsername_ != username) {
        return false;
    }

    UserAccount* account = repository_.find(username);
    if (account == nullptr) {
        return false;
    }

    auto storedPath = ProfileImageStore::store(username, sourceImagePath);
    if (!storedPath.has_value()) {
        return false;
    }

    account->profileImagePath = *storedPath;
    return true;
}

std::optional<std::string> UX::getProfileImagePath(const std::string& username) const {
    const UserAccount* account = repository_.find(username);
    if (account == nullptr || account->profileImagePath.empty()) {
        return std::nullopt;
    }
    return account->profileImagePath;
}

bool UX::isPasswordStrong(const std::string& password) {
    return PasswordSecurity::isStrong(password);
}

bool UX::saveToFile(const std::string& filePath) const {
    return repository_.saveToFile(filePath);
}

bool UX::loadFromFile(const std::string& filePath) {
    currentUsername_.reset();
    return repository_.loadFromFile(filePath);
}
