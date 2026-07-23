#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "BudgetBitesLib/UserRepository.h"

using namespace std;

namespace {

UserAccount makeAccount(const string& username) {
    UserAccount account;
    account.username = username;
    account.passwordHash = "knucklehead";
    account.passwordSalt = "0liver";
    account.profileImagePath = "profile_images/" + username + ".png";
    return account;
}

}

TEST_CASE("a new UserRepository starts out empty", "[UserRepo]") {
    UserRepository repo;
    CHECK(repo.size() == 0);
    CHECK(repo.find("anyone") == nullptr);
}

TEST_CASE("add lets you find the account afterwards", "[UserRepo]") {
    UserRepository repo;
    repo.add(makeAccount("Nikki"));

    REQUIRE(repo.size() == 1);
    UserAccount* found = repo.find("Nikki");
    REQUIRE(found != nullptr);
    CHECK(found->username == "Nikki");
    CHECK(found->passwordHash == "knucklehead");
}

TEST_CASE("find isn't case sensitive", "[UserRepo]") {
    UserRepository repo;
    repo.add(makeAccount("Nikki"));

    CHECK(repo.find("nikki") != nullptr);
    CHECK(repo.find("NIKKI") != nullptr);
    CHECK(repo.find("nIKKi") != nullptr);
}

TEST_CASE("find returns nullptr if the username was never added", "[UserRepo]") {
    UserRepository repo;
    repo.add(makeAccount("Nikki"));

    CHECK(repo.find("Michael") == nullptr);
}

TEST_CASE("loadFromFile returns false if the file doesn't exist yet", "[UserRepo]") {
    string path = "nonexistent_users.txt";
    filesystem::remove(path);

    UserRepository repo;
    CHECK_FALSE(repo.loadFromFile(path));
}

TEST_CASE("loadFromFile just skips lines that aren't formatted right", "[UserRepo]") {
    // the file format is pretty simple - if a line doesn't split into
    // exactly 4 comma separated fields we just skip it, we don't fail
    // the whole load
    string path = "corrupt_users.txt";
    {
        ofstream out(path);
        out << "not a real user line\n";
    }

    UserRepository repo;
    bool loaded = repo.loadFromFile(path);

    CHECK(loaded);
    CHECK(repo.size() == 0);

    filesystem::remove(path);
}

TEST_CASE("saveToFile then loadFromFile gives back the same accounts", "[UserRepo]") {
    string path = "roundtrip_users.txt";
    filesystem::remove(path);

    UserRepository original;
    original.add(makeAccount("Nikki"));
    original.add(makeAccount("Michael"));
    REQUIRE(original.saveToFile(path));

    UserRepository loaded;
    REQUIRE(loaded.loadFromFile(path));
    REQUIRE(loaded.size() == 2);

    UserAccount* nikki = loaded.find("Nikki");
    REQUIRE(nikki != nullptr);
    CHECK(nikki->passwordHash == "knucklehead");
    CHECK(nikki->passwordSalt == "0liver");
    CHECK(nikki->profileImagePath == "profile_images/Nikki.png");

    CHECK(loaded.find("Michael") != nullptr);

    filesystem::remove(path);
}

TEST_CASE("loadFromFile replaces whatever was already loaded", "[UserRepo]") {
    string path = "replace_users.txt";
    filesystem::remove(path);

    UserRepository saved;
    saved.add(makeAccount("OnlyOnDisk"));
    REQUIRE(saved.saveToFile(path));

    UserRepository repo;
    repo.add(makeAccount("AlreadyInMemory"));
    REQUIRE(repo.loadFromFile(path));

    CHECK(repo.size() == 1);
    CHECK(repo.find("OnlyOnDisk") != nullptr);
    CHECK(repo.find("AlreadyInMemory") == nullptr);

    filesystem::remove(path);
}