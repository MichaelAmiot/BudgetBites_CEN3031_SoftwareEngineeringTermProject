#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/UX.h"

using namespace std;
namespace fs = std::filesystem;

const string strongPassword = "Str0ng!Pass";

TEST_CASE("registerUser rejects an empty username", "[ux]") {
    UX ux;
    CHECK_FALSE(ux.registerUser("", strongPassword));
}

TEST_CASE("registerUser rejects a weak password", "[ux]") {
    UX ux;
    CHECK_FALSE(ux.registerUser("Nikki", "weak"));
}

TEST_CASE("registerUser works for a new username with a strong password", "[ux]") {
    UX ux;
    CHECK(ux.registerUser("Nikki", strongPassword));
}

TEST_CASE("registerUser rejects a username that's already taken (case-insensitive)", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    CHECK_FALSE(ux.registerUser("Nikki", strongPassword));
    CHECK_FALSE(ux.registerUser("nikki", strongPassword));
    CHECK_FALSE(ux.registerUser("NIKKI", strongPassword));
}

TEST_CASE("signIn fails if the username was never registered", "[ux]") {
    UX ux;
    CHECK_FALSE(ux.signIn("nobody", strongPassword));
    CHECK_FALSE(ux.isSignedIn());
}

TEST_CASE("signIn fails with the wrong password", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    CHECK_FALSE(ux.signIn("Nikki", "WrongPassword1!"));
    CHECK_FALSE(ux.isSignedIn());
}

TEST_CASE("signIn works and starts a session with the right credentials", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    REQUIRE(ux.signIn("Nikki", strongPassword));
    CHECK(ux.isSignedIn());
    REQUIRE(ux.currentUser().has_value());
    CHECK(*ux.currentUser() == "Nikki");
}

TEST_CASE("signOut ends the session", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));
    REQUIRE(ux.signIn("Nikki", strongPassword));

    ux.signOut();

    CHECK_FALSE(ux.isSignedIn());
    CHECK_FALSE(ux.currentUser().has_value());
}

TEST_CASE("isPasswordStrong agrees with what registerUser checks", "[ux]") {
    CHECK(UX::isPasswordStrong(strongPassword));
    CHECK_FALSE(UX::isPasswordStrong("weak"));
}

TEST_CASE("uploadProfileImage fails if nobody is signed in", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    string path = "ux_upload_no_session.png";
    fs::remove(path);
    ofstream out(path);
    out << "pretend-image-bytes";
    out.close();

    CHECK_FALSE(ux.uploadProfileImage("Nikki", path));

    fs::remove(path);
}

TEST_CASE("uploadProfileImage fails when uploading for someone else", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));
    REQUIRE(ux.registerUser("Michael", strongPassword));
    REQUIRE(ux.signIn("Nikki", strongPassword));

    string path = "ux_upload_wrong_user.png";
    fs::remove(path);
    ofstream out(path);
    out << "pretend-image-bytes";
    out.close();

    CHECK_FALSE(ux.uploadProfileImage("Michael", path));

    fs::remove(path);
}

TEST_CASE("uploadProfileImage works for your own account", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));
    REQUIRE(ux.signIn("Nikki", strongPassword));

    string path = "ux_upload_success.png";
    fs::remove(path);
    ofstream out(path);
    out << "pretend-image-bytes";
    out.close();

    bool uploaded = ux.uploadProfileImage("Nikki", path);
    CHECK(uploaded);

    auto storedPath = ux.getProfileImagePath("Nikki");
    REQUIRE(storedPath.has_value());
    CHECK(fs::exists(*storedPath));

    fs::remove(path);
    fs::remove(*storedPath);
}

TEST_CASE("getProfileImagePath has nothing if no picture was uploaded", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    CHECK_FALSE(ux.getProfileImagePath("Nikki").has_value());
}

TEST_CASE("getProfileImagePath has nothing for someone who doesn't exist", "[ux]") {
    UX ux;
    CHECK_FALSE(ux.getProfileImagePath("nobody").has_value());
}

TEST_CASE("saveToFile then loadFromFile keeps accounts working", "[ux]") {
    string path = "ux_roundtrip.txt";
    filesystem::remove(path);

    UX original;
    REQUIRE(original.registerUser("Nikki", strongPassword));
    REQUIRE(original.saveToFile(path));

    UX loaded;
    REQUIRE(loaded.loadFromFile(path));

    CHECK(loaded.signIn("Nikki", strongPassword));

    filesystem::remove(path);
}

TEST_CASE("loadFromFile signs everyone out", "[ux]") {
    string path = "ux_load_clears_session.txt";
    filesystem::remove(path);

    UX ux;
    REQUIRE(ux.registerUser("Nikki", strongPassword));
    REQUIRE(ux.signIn("Nikki", strongPassword));
    REQUIRE(ux.saveToFile(path));
    REQUIRE(ux.isSignedIn());

    REQUIRE(ux.loadFromFile(path));

    CHECK_FALSE(ux.isSignedIn());

    filesystem::remove(path);
}

TEST_CASE("current user supplemental information is saved and loaded", "[ux]") {
    const auto directory = filesystem::temp_directory_path() / "budgetbites-ux-user-info-test";
    filesystem::remove_all(directory);

    UX ux(directory);
    REQUIRE(ux.registerUser("Nikki", strongPassword));
    REQUIRE(ux.signIn("Nikki", strongPassword));

    Account account;
    account.setAllergenIds({1, 6});
    Preferences preferences;
    REQUIRE(preferences.setBudget(45.0));
    preferences.setDietaryTagIds({1, 4});
    Ingredients ingredients;
    REQUIRE(ingredients.addIngredient(22, 300.0));
    REQUIRE(ingredients.addIngredient(153));
    REQUIRE(ux.saveCurrentUserInfo(account, preferences, ingredients));

    Account loadedAccount;
    Preferences loadedPreferences;
    Ingredients loadedIngredients;
    REQUIRE(ux.loadCurrentUserInfo(loadedAccount, loadedPreferences, loadedIngredients));

    CHECK(loadedAccount.getAllergenIds() == vector<int>{1, 6});
    CHECK(loadedPreferences.getDietaryTagIds() == vector<int>{1, 4});
    CHECK(loadedPreferences.getBudget() == 45.0);
    REQUIRE(loadedIngredients.getPantryItems().size() == 2);
    CHECK(loadedIngredients.getPantryItems()[0].ingredientId == 22);
    CHECK(loadedIngredients.getPantryItems()[0].availableGrams == 300.0);
    CHECK(loadedIngredients.getPantryItems()[1].ingredientId == 153);
    CHECK_FALSE(loadedIngredients.getPantryItems()[1].availableGrams.has_value());

    filesystem::remove_all(directory);
}
