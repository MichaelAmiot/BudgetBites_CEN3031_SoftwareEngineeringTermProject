#include "BudgetBitesLib/UX.h"

#include "BudgetBitesLib/PasswordSecurity.h"
#include "BudgetBitesLib/ProfileImageStore.h"

UX::UX(const std::filesystem::path& storageDirectory)
    : repository(storageDirectory) {}

bool UX::registerUser(const std::string& username, const std::string& password) {
    if (username.empty() || repository.getUserByUsername(username) || !PasswordSecurity::isStrong(password)) {
        return false;
    }
    const std::string salt = PasswordSecurity::generateSalt();
    const auto user = repository.createUser(username, PasswordSecurity::hashPassword(password, salt), salt);
    return user.has_value() && repository.save();
}

bool UX::signIn(const std::string& username, const std::string& password) {
    const auto user = repository.getUserByUsername(username);
    if (!user || !PasswordSecurity::checkPassword(password, user->passwordSalt, user->passwordHash)) {
        return false;
    }
    // Keep the stable user ID in the session instead of the display name.
    currentUserId = user->id;
    return true;
}

void UX::signOut() {
    currentUserId.reset();
}

bool UX::isSignedIn() const {
    return currentUserId.has_value();
}

std::optional<std::string> UX::currentUser() const {
    const auto user = currentUserInfo();
    return user ? std::optional<std::string>(user->username) : std::nullopt;
}

std::optional<UserInfo> UX::currentUserInfo() const {
    return currentUserId ? repository.getUserById(*currentUserId) : std::nullopt;
}

bool UX::uploadProfileImage(const std::string& username, const std::string& sourceImagePath) {
    const auto signedInUser = currentUser();
    if (!signedInUser || *signedInUser != username) {
        return false;
    }
    const auto storedPath = ProfileImageStore::store(username, sourceImagePath);
    return storedPath && repository.updateProfileImagePath(*currentUserId, *storedPath) && repository.save();
}

std::optional<std::string> UX::getProfileImagePath(const std::string& username) const {
    const auto user = repository.getUserByUsername(username);
    if (!user || user->profileImagePath.empty()) {
        return std::nullopt;
    }
    return user->profileImagePath;
}

bool UX::updateCurrentWeeklyBudget(double weeklyBudget) {
    return currentUserId && repository.updateWeeklyBudget(*currentUserId, weeklyBudget) && repository.save();
}

bool UX::replaceCurrentDietaryTagIds(const std::vector<int>& dietaryTagIds) {
    return currentUserId && repository.replaceDietaryTagIds(*currentUserId, dietaryTagIds) && repository.save();
}

bool UX::replaceCurrentAllergenIds(const std::vector<int>& allergenIds) {
    return currentUserId && repository.replaceAllergenIds(*currentUserId, allergenIds) && repository.save();
}

bool UX::replaceCurrentPantryItems(const std::vector<PantryItem>& pantryItems) {
    return currentUserId && repository.replacePantryItems(*currentUserId, pantryItems) && repository.save();
}

std::vector<int> UX::getCurrentDietaryTagIds() const {
    return currentUserId ? repository.getDietaryTagIds(*currentUserId) : std::vector<int>{};
}

std::vector<int> UX::getCurrentAllergenIds() const {
    return currentUserId ? repository.getAllergenIds(*currentUserId) : std::vector<int>{};
}

std::vector<PantryItem> UX::getCurrentPantryItems() const {
    return currentUserId ? repository.getPantryItems(*currentUserId) : std::vector<PantryItem>{};
}

bool UX::saveUserData() {
    return repository.save();
}

bool UX::reloadUserData() {
    currentUserId.reset();
    return repository.load();
}

bool UX::isPasswordStrong(const std::string& password) {
    return PasswordSecurity::isStrong(password);
}
