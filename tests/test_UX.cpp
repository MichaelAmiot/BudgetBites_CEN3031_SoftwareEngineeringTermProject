#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "BudgetBitesLib/UX.h"

namespace {

namespace fs = std::filesystem;

constexpr const char* kStrongPassword = "Str0ng!Pass";

// Removes a file (if present) on construction and destruction, so tests
// never depend on -- or leave behind -- state from a previous run.
struct TempFile {
    fs::path path;
    explicit TempFile(const std::string& name) : path(name) {
        fs::remove(path);
    }
    ~TempFile() {
        fs::remove(path);
    }
};

// Same idea as TempFile, but also tracks and removes the copy that
// ProfileImageStore::store() creates under profile_images/.
struct TempImage {
    fs::path sourcePath;
    fs::path storedPath;
    explicit TempImage(const fs::path& source) : sourcePath(source) {
        fs::remove(sourcePath);
        std::ofstream out(sourcePath, std::ios::binary);
        out << "pretend-image-bytes";
    }
    ~TempImage() {
        fs::remove(sourcePath);
        if (!storedPath.empty()) {
            fs::remove(storedPath);
        }
    }
};

} // namespace

TEST_CASE("registerUser rejects an empty username", "[ux]") {
    UX ux;
    CHECK_FALSE(ux.registerUser("", kStrongPassword));
}

TEST_CASE("registerUser rejects a password that fails the complexity policy", "[ux]") {
    UX ux;
    CHECK_FALSE(ux.registerUser("Alice", "weak"));
}

TEST_CASE("registerUser succeeds for a unique username and strong password", "[ux]") {
    UX ux;
    CHECK(ux.registerUser("Alice", kStrongPassword));
}

TEST_CASE("registerUser rejects a username that is already taken, case-insensitively", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));

    CHECK_FALSE(ux.registerUser("Alice", kStrongPassword));
    CHECK_FALSE(ux.registerUser("alice", kStrongPassword));
    CHECK_FALSE(ux.registerUser("ALICE", kStrongPassword));
}

TEST_CASE("signIn fails for a username that was never registered", "[ux]") {
    UX ux;
    CHECK_FALSE(ux.signIn("nobody", kStrongPassword));
    CHECK_FALSE(ux.isSignedIn());
}

TEST_CASE("signIn fails for the wrong password", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));

    CHECK_FALSE(ux.signIn("Alice", "WrongPassword1!"));
    CHECK_FALSE(ux.isSignedIn());
}

TEST_CASE("signIn succeeds with the correct credentials and starts a session", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));

    REQUIRE(ux.signIn("Alice", kStrongPassword));
    CHECK(ux.isSignedIn());
    REQUIRE(ux.currentUser().has_value());
    CHECK(*ux.currentUser() == "Alice");
}

TEST_CASE("signOut clears the current session", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));
    REQUIRE(ux.signIn("Alice", kStrongPassword));

    ux.signOut();

    CHECK_FALSE(ux.isSignedIn());
    CHECK_FALSE(ux.currentUser().has_value());
}

TEST_CASE("isPasswordStrong delegates to the same complexity policy as registerUser", "[ux]") {
    CHECK(UX::isPasswordStrong(kStrongPassword));
    CHECK_FALSE(UX::isPasswordStrong("weak"));
}

TEST_CASE("uploadProfileImage fails when no one is signed in", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));
    TempImage image("ux_upload_no_session.png");

    CHECK_FALSE(ux.uploadProfileImage("Alice", image.sourcePath.string()));
}

TEST_CASE("uploadProfileImage fails when uploading for a different user than the signed-in one", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));
    REQUIRE(ux.registerUser("Bob", kStrongPassword));
    REQUIRE(ux.signIn("Alice", kStrongPassword));
    TempImage image("ux_upload_wrong_user.png");

    CHECK_FALSE(ux.uploadProfileImage("Bob", image.sourcePath.string()));
}

TEST_CASE("uploadProfileImage succeeds for the signed-in user's own account", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));
    REQUIRE(ux.signIn("Alice", kStrongPassword));
    TempImage image("ux_upload_success.png");

    bool uploaded = ux.uploadProfileImage("Alice", image.sourcePath.string());
    if (uploaded) {
        image.storedPath = *ux.getProfileImagePath("Alice");
    }

    CHECK(uploaded);
    REQUIRE(ux.getProfileImagePath("Alice").has_value());
    CHECK(fs::exists(*ux.getProfileImagePath("Alice")));
}

TEST_CASE("getProfileImagePath returns nullopt when no image has been uploaded", "[ux]") {
    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));

    CHECK_FALSE(ux.getProfileImagePath("Alice").has_value());
}

TEST_CASE("getProfileImagePath returns nullopt for an unknown username", "[ux]") {
    UX ux;
    CHECK_FALSE(ux.getProfileImagePath("nobody").has_value());
}

TEST_CASE("saveToFile then loadFromFile round-trips registered accounts", "[ux]") {
    TempFile temp("ux_roundtrip.dat");

    UX original;
    REQUIRE(original.registerUser("Alice", kStrongPassword));
    REQUIRE(original.saveToFile(temp.path.string()));

    UX loaded;
    REQUIRE(loaded.loadFromFile(temp.path.string()));

    CHECK(loaded.signIn("Alice", kStrongPassword));
}

TEST_CASE("loadFromFile clears any existing signed-in session", "[ux]") {
    TempFile temp("ux_load_clears_session.dat");

    UX ux;
    REQUIRE(ux.registerUser("Alice", kStrongPassword));
    REQUIRE(ux.signIn("Alice", kStrongPassword));
    REQUIRE(ux.saveToFile(temp.path.string()));
    REQUIRE(ux.isSignedIn());

    REQUIRE(ux.loadFromFile(temp.path.string()));

    CHECK_FALSE(ux.isSignedIn());
}
