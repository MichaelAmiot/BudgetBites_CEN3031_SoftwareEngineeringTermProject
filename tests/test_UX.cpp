#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "BudgetBitesLib/UX.h"

using namespace std;
namespace fs = std::filesystem;

const string strongPassword = "Str0ng!Pass";

fs::path nextStorageDirectory() {
    static int directoryNumber = 0;
    const fs::path directory = fs::temp_directory_path() /
        ("budgetbites-ux-test-" + to_string(directoryNumber++));
    fs::remove_all(directory);
    return directory;
}

TEST_CASE("registerUser rejects an empty username", "[ux]") {
    UX ux(nextStorageDirectory());
    CHECK_FALSE(ux.registerUser("", strongPassword));
}

TEST_CASE("registerUser rejects a weak password", "[ux]") {
    UX ux(nextStorageDirectory());
    CHECK_FALSE(ux.registerUser("Nikki", "weak"));
}

TEST_CASE("registerUser works for a new username with a strong password", "[ux]") {
    UX ux(nextStorageDirectory());
    CHECK(ux.registerUser("Nikki", strongPassword));
}

TEST_CASE("registerUser rejects a username that's already taken (case-insensitive)", "[ux]") {
    UX ux(nextStorageDirectory());
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    CHECK_FALSE(ux.registerUser("Nikki", strongPassword));
    CHECK_FALSE(ux.registerUser("nikki", strongPassword));
    CHECK_FALSE(ux.registerUser("NIKKI", strongPassword));
}

TEST_CASE("signIn fails if the username was never registered", "[ux]") {
    UX ux(nextStorageDirectory());
    CHECK_FALSE(ux.signIn("nobody", strongPassword));
    CHECK_FALSE(ux.isSignedIn());
}

TEST_CASE("signIn fails with the wrong password", "[ux]") {
    UX ux(nextStorageDirectory());
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    CHECK_FALSE(ux.signIn("Nikki", "WrongPassword1!"));
    CHECK_FALSE(ux.isSignedIn());
}

TEST_CASE("signIn works and starts a session with the right credentials", "[ux]") {
    UX ux(nextStorageDirectory());
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    REQUIRE(ux.signIn("Nikki", strongPassword));
    CHECK(ux.isSignedIn());
    REQUIRE(ux.currentUser().has_value());
    CHECK(*ux.currentUser() == "Nikki");
}

TEST_CASE("signOut ends the session", "[ux]") {
    UX ux(nextStorageDirectory());
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
    UX ux(nextStorageDirectory());
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
    UX ux(nextStorageDirectory());
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
    UX ux(nextStorageDirectory());
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
    UX ux(nextStorageDirectory());
    REQUIRE(ux.registerUser("Nikki", strongPassword));

    CHECK_FALSE(ux.getProfileImagePath("Nikki").has_value());
}

TEST_CASE("getProfileImagePath has nothing for someone who doesn't exist", "[ux]") {
    UX ux(nextStorageDirectory());
    CHECK_FALSE(ux.getProfileImagePath("nobody").has_value());
}

TEST_CASE("saveUserData then reloadUserData keeps accounts working", "[ux]") {
    const fs::path storageDirectory = nextStorageDirectory();
    UX original(storageDirectory);
    REQUIRE(original.registerUser("Nikki", strongPassword));
    REQUIRE(original.saveUserData());

    UX loaded(storageDirectory);
    REQUIRE(loaded.reloadUserData());

    CHECK(loaded.signIn("Nikki", strongPassword));
    fs::remove_all(storageDirectory);
}

TEST_CASE("reloadUserData signs everyone out", "[ux]") {
    const fs::path storageDirectory = nextStorageDirectory();
    UX ux(storageDirectory);
    REQUIRE(ux.registerUser("Nikki", strongPassword));
    REQUIRE(ux.signIn("Nikki", strongPassword));
    REQUIRE(ux.saveUserData());
    REQUIRE(ux.isSignedIn());

    REQUIRE(ux.reloadUserData());

    CHECK_FALSE(ux.isSignedIn());
    fs::remove_all(storageDirectory);
}

TEST_CASE("signed-in users persist their profile data through UX", "[ux]") {
    const fs::path storageDirectory = nextStorageDirectory();
    UX ux(storageDirectory);
    REQUIRE(ux.registerUser("Nikki", strongPassword));
    REQUIRE(ux.signIn("Nikki", strongPassword));
    REQUIRE(ux.updateCurrentWeeklyBudget(45.0));
    REQUIRE(ux.replaceCurrentDietaryTagIds({1, 4}));
    REQUIRE(ux.replaceCurrentAllergenIds({2}));
    REQUIRE(ux.replaceCurrentPantryItems({{22, 300.0}}));

    UX loaded(storageDirectory);
    REQUIRE(loaded.signIn("Nikki", strongPassword));
    REQUIRE(loaded.currentUserInfo().has_value());
    CHECK(loaded.currentUserInfo()->weeklyBudget == 45.0);
    CHECK(loaded.getCurrentDietaryTagIds() == vector<int>{1, 4});
    CHECK(loaded.getCurrentAllergenIds() == vector<int>{2});
    CHECK(loaded.getCurrentPantryItems().size() == 1);

    fs::remove_all(storageDirectory);
}
