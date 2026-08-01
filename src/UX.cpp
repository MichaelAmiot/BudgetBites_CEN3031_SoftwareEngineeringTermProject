#include "BudgetBitesLib/UX.h"

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/PasswordSecurity.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/ProfileImageStore.h"

using namespace std;

bool UX::registerUser(const string& username, const string& password) {
    // Account records use commas and lines as delimiters, so usernames cannot contain them.
    if (username.empty() || username.find_first_of(",\r\n") != string::npos) {
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

bool UX::loadCurrentUserInfo(
    Account& account,
    Preferences& preferences,
    Ingredients& ingredients
) {
    if (!currentUsername || !userInfoRepository.ensureUser(*currentUsername)) {
        return false;
    }

    const auto extraInfo = userInfoRepository.getUserByUsername(*currentUsername);
    if (!extraInfo) {
        return false;
    }

    // The repository stores catalog IDs; the business objects keep those same IDs in memory.
    preferences.setBudget(extraInfo->weeklyBudget);
    preferences.setDietaryTagIds(userInfoRepository.getDietaryTagIds(*currentUsername));
    account.setAllergenIds(userInfoRepository.getAllergenIds(*currentUsername));
    ingredients.setPantryItems(userInfoRepository.getPantryItems(*currentUsername));
    return true;
}

bool UX::saveCurrentUserInfo(
    const Account& account,
    const Preferences& preferences,
    const Ingredients& ingredients
) {
    if (!currentUsername || !userInfoRepository.ensureUser(*currentUsername)) {
        return false;
    }

    // Replace each saved list with the current selections before writing local CSV files.
    return userInfoRepository.updateWeeklyBudget(*currentUsername, preferences.getBudget()) &&
           userInfoRepository.replaceDietaryTagIds(*currentUsername, preferences.getDietaryTagIds()) &&
           userInfoRepository.replaceAllergenIds(*currentUsername, account.getAllergenIds()) &&
           userInfoRepository.replacePantryItems(*currentUsername, ingredients.getPantryItems()) &&
           userInfoRepository.save();
}

bool UX::saveToFile(const string& filePath) const {
    return repository.saveToFile(filePath);
}

bool UX::loadFromFile(const string& filePath) {
    currentUsername.reset();
    return repository.loadFromFile(filePath);
}
