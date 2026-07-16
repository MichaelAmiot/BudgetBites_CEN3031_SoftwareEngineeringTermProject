#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "BudgetBitesLib/UserRepository.h"

namespace {

// Keeps a temp file's path around and removes it (and any leftovers from
// a previous failed run) when the test goes out of scope, so tests never
// depend on files left behind by earlier runs.
struct TempFile {
    std::filesystem::path path;
    explicit TempFile(const std::string& name) : path(name) {
        std::filesystem::remove(path);
    }
    ~TempFile() {
        std::filesystem::remove(path);
    }
};

UserAccount makeAccount(const std::string& username) {
    UserAccount account;
    account.username = username;
    account.passwordHash = "deadbeef";
    account.passwordSalt = "cafef00d";
    account.profileImagePath = "profile_images/" + username + ".png";
    return account;
}

} // namespace

TEST_CASE("A freshly constructed UserRepository is empty", "[UserRepo]") {
    UserRepository repo;
    CHECK(repo.size() == 0);
    CHECK(repo.find("anyone") == nullptr);
}

TEST_CASE("add() stores an account that find() can then retrieve", "[UserRepo]") {
    UserRepository repo;
    repo.add(makeAccount("Alice"));

    REQUIRE(repo.size() == 1);
    const UserAccount* found = repo.find("Alice");
    REQUIRE(found != nullptr);
    CHECK(found->username == "Alice");
    CHECK(found->passwordHash == "deadbeef");
}

TEST_CASE("find() is case-insensitive", "[UserRepo]") {
    UserRepository repo;
    repo.add(makeAccount("Alice"));

    CHECK(repo.find("alice") != nullptr);
    CHECK(repo.find("ALICE") != nullptr);
    CHECK(repo.find("aLiCe") != nullptr);
}

TEST_CASE("find() returns nullptr for a username that was never added", "[UserRepo]") {
    UserRepository repo;
    repo.add(makeAccount("Alice"));

    CHECK(repo.find("Bob") == nullptr);
}

TEST_CASE("loadFromFile fails gracefully when the file doesn't exist", "[UserRepo]") {
    TempFile temp("nonexistent_users.dat");
    UserRepository repo;

    CHECK_FALSE(repo.loadFromFile(temp.path.string()));
}

TEST_CASE("loadFromFile fails gracefully on a corrupt/truncated file", "[UserRepo]") {
    TempFile temp("corrupt_users.dat");
    {
        std::ofstream out(temp.path, std::ios::binary);
        out << "not a real user file";
    }

    UserRepository repo;
    CHECK_FALSE(repo.loadFromFile(temp.path.string()));
}

TEST_CASE("saveToFile then loadFromFile round-trips every account field", "[UserRepo]") {
    TempFile temp("roundtrip_users.dat");

    UserRepository original;
    original.add(makeAccount("Alice"));
    original.add(makeAccount("Bob"));
    REQUIRE(original.saveToFile(temp.path.string()));

    UserRepository loaded;
    REQUIRE(loaded.loadFromFile(temp.path.string()));
    REQUIRE(loaded.size() == 2);

    const UserAccount* alice = loaded.find("Alice");
    REQUIRE(alice != nullptr);
    CHECK(alice->passwordHash == "deadbeef");
    CHECK(alice->passwordSalt == "cafef00d");
    CHECK(alice->profileImagePath == "profile_images/Alice.png");

    CHECK(loaded.find("Bob") != nullptr);
}

TEST_CASE("loadFromFile replaces in-memory accounts rather than appending", "[UserRepo]") {
    TempFile temp("replace_users.dat");

    UserRepository saved;
    saved.add(makeAccount("OnlyOnDisk"));
    REQUIRE(saved.saveToFile(temp.path.string()));

    UserRepository repo;
    repo.add(makeAccount("AlreadyInMemory"));
    REQUIRE(repo.loadFromFile(temp.path.string()));

    CHECK(repo.size() == 1);
    CHECK(repo.find("OnlyOnDisk") != nullptr);
    CHECK(repo.find("AlreadyInMemory") == nullptr);
}
