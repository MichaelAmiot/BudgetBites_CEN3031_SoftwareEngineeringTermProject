#include "BudgetBitesLib/UX.h"

#include "BudgetBitesLib/PasswordSecurity.h"
#include "BudgetBitesLib/ProfileImageStore.h"

using namespace std;

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

bool UX::registerUser(const string& username, const string& password) {
    if (username.empty()) {
        return false;
    }
    if (repository.find(username) != nullptr) {
        return false; // username already taken
    }
    if (!PasswordSecurity::isStrong(password)) {
        return false;
    }

    UserAccount account;
    account.username = username;
    account.passwordSalt = PasswordSecurity::generateSalt();
    account.passwordHash = PasswordSecurity::hashPassword(password, account.passwordSalt);

    repository.add(account);
    return true;
}

bool UX::signIn(const string& username, const string& password) {
    UserAccount* account = repository.find(username);
    if (account == nullptr) {
        return false;
    }

    if (!PasswordSecurity::checkPassword(password, account->passwordSalt, account->passwordHash)) {
        return false;
    }

    currentUsername = username;
    return true;
}

void UX::signOut() {
    currentUsername.reset();
}

bool UX::isSignedIn() const {
    return currentUsername.has_value();
}

optional<string> UX::currentUser() const {
    return currentUsername;
}

bool UX::uploadProfileImage(const string& username, const string& sourceImagePath) {
    // only let someone upload a picture for their own account
    if (!currentUsername.has_value() || currentUsername != username) {
        return false;
    }

    UserAccount* account = repository.find(username);
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

optional<string> UX::getProfileImagePath(const string& username) {
    UserAccount* account = repository.find(username);
    if (account == nullptr || account->profileImagePath.empty()) {
        return nullopt;
    }
    return account->profileImagePath;
}

bool UX::isPasswordStrong(const string& password) {
    return PasswordSecurity::isStrong(password);
}

bool UX::saveToFile(const string& filePath) const {
    return repository.saveToFile(filePath);
}

bool UX::loadFromFile(const string& filePath) {
    currentUsername.reset();
    return repository.loadFromFile(filePath);
}