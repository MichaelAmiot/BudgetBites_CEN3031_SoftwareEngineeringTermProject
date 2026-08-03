#include "BudgetBitesLib/UX.h"

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/PasswordSecurity.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/ProfileImageStore.h"

using namespace std;

// Lets the caller check if a username is free before the person even
// picks a password, so we can warn them right away instead of waiting
// until the whole registration form is filled out.
bool UX::isUsernameTaken(const string& username) {
    return repository.exists(username);
}

// Creates a brand new account if the username and password both pass
// our rules. Fails if the username is empty, contains characters our
// CSV storage cannot handle, is already taken, or if the password is
// not strong enough. On success the password is salted and hashed
// before it ever gets stored.
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

// Checks the username and password against what is stored and, if they
//  match, remembers that this user is now signed in for the rest of the
//  session.
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

// Clears whoever is currently signed in.
void UX::signOut() {
    currentUsername.reset();
}

// True if somebody is currently signed in.
bool UX::isSignedIn() const {
    return currentUsername.has_value();
}

// Gives back the signed in username, or nothing if no one is signed in.
optional<string> UX::currentUser() const {
    return currentUsername;
}

// Saves a profile picture for the given user, but only goes through if
//  that user is the one currently signed in and actually has an account.
// The real copying and file checks happen over in ProfileImageStore.
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

bool UX::removeProfileImage(const string& username) {
    // only let someone remove the picture on their own account
    if (!currentUsername.has_value() || currentUsername != username) {
        return false;
    }

    UserAccount* account = repository.find(username);
    if (account == nullptr) {
        return false;
    }

    if (account->profileImagePath.empty()) {
        return true; // nothing to remove, already in the state we want
    }

    if (!ProfileImageStore::remove(account->profileImagePath)) {
        return false;
    }

    account->profileImagePath.clear();
    return true;
}

// Looks up where a user's profile picture is saved, if they have one.
optional<string> UX::getProfileImagePath(const string& username) {
    UserAccount* account = repository.find(username);
    if (account == nullptr || account->profileImagePath.empty()) {
        return nullopt;
    }
    return account->profileImagePath;
}

// Small wrapper so other parts of the app can check password strength
//  without needing to know PasswordSecurity exists.
bool UX::isPasswordStrong(const string& password) {
    return PasswordSecurity::isStrong(password);
}

// Pulls the signed in user's saved budget, dietary tags, allergens, and
//  pantry items out of storage and fills them into the objects the rest
//  of the app works with. Fails if nobody is signed in or if their saved
//  data cannot be found.
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

// Takes whatever the signed in user has set for their budget, dietary
//  tags, allergens, and pantry items, and writes all of it back out to
//  storage. Stops and returns false as soon as any one of those saves fails.
bool UX::saveCurrentUserInfo(
    const Account& account,
    const Preferences& preferences,
    const Ingredients& ingredients
) {
    if (!currentUsername || !userInfoRepository.ensureUser(*currentUsername)) {
        return false;
    }

    if (!userInfoRepository.updateWeeklyBudget(*currentUsername, preferences.getBudget())) {
        return false;
    }
    if (!userInfoRepository.replaceDietaryTagIds(
            *currentUsername,
            preferences.getDietaryTagIds())) {
        return false;
    }
    if (!userInfoRepository.replaceAllergenIds(*currentUsername, account.getAllergenIds())) {
        return false;
    }
    if (!userInfoRepository.replacePantryItems(*currentUsername, ingredients.getPantryItems())) {
        return false;
    }
    return userInfoRepository.save();
}

// Saves the account list (usernames, password hashes, and so on) to a file.
bool UX::saveToFile(const string& filePath) const {
    return repository.saveToFile(filePath);
}

// Loads the account list back in from a file. Signs everyone out first,
// since whatever session was active no longer matches whatever gets loaded.
bool UX::loadFromFile(const string& filePath) {
    currentUsername.reset();
    return repository.loadFromFile(filePath);
}
