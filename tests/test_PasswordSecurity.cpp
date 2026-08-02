#include <catch2/catch_test_macros.hpp>
#include "BudgetBitesLib/PasswordSecurity.h"
#include <string>

using namespace PasswordSecurity;
using namespace std;

TEST_CASE("isStrong rejects passwords missing something", "[PasswordSecurity]") {
    CHECK_FALSE(isStrong("Ab1!"));           // too short
    CHECK_FALSE(isStrong("lowercase1!"));    // no uppercase
    CHECK_FALSE(isStrong("UPPERCASE1!"));    // no lowercase
    CHECK_FALSE(isStrong("NoDigitsHere!"));  // no number
    CHECK_FALSE(isStrong("NoSpecial123"));   // no symbol
}

TEST_CASE("isStrong accepts a password with everything required", "[PasswordSecurity]") {
    REQUIRE(isStrong("Str0ng!Pass"));
}

TEST_CASE("generateSalt returns an 8 character string", "[PasswordSecurity]") {
    string salt = generateSalt();
    CHECK(salt.length() == 8);
}

TEST_CASE("generateSalt gives different results each time", "[PasswordSecurity]") {
    // technically two random salts COULD match but with 8 random
    // characters that's basically never going to happen
    REQUIRE(generateSalt() != generateSalt());
}

TEST_CASE("hashPassword gives same result for same password and salt", "[PasswordSecurity]") {
    string salt = generateSalt();
    REQUIRE(hashPassword("cash-dog-girl-car", salt) == hashPassword("cash-dog-girl-car", salt));
}

TEST_CASE("hashPassword changes when salt changes", "[PasswordSecurity]") {
    // Synthetic test credential — not a real password, used only to exercise validation logic
    const string kSyntheticTestCredential = "cash-dog-girl-car";
    string password = kSyntheticTestCredential;
    CHECK(hashPassword(password, "aaaa") != hashPassword(password, "bbbb"));
}

TEST_CASE("hashPassword changes when password changes", "[PasswordSecurity]") {
    string salt = generateSalt();
    CHECK(hashPassword("password-one", salt) != hashPassword("password-two", salt));
}

TEST_CASE("checkPassword works with correct password and fails with wrong one", "[PasswordSecurity]") {
    string salt = generateSalt();
    string storedHash = hashPassword("TheReal Password1!", salt);

    CHECK(checkPassword("TheReal Password1!", salt, storedHash));
    CHECK_FALSE(checkPassword("wrong-password", salt, storedHash));
}

TEST_CASE("checkPassword fails if the hash was changed", "[PasswordSecurity]") {
    string salt = generateSalt();
    string storedHash = hashPassword("TheReal Password1!", salt);

    // just flip the last character to simulate the hash getting messed up
    storedHash.back() = (storedHash.back() == '0') ? '1' : '0';

    CHECK_FALSE(checkPassword("TheReal Password1!", salt, storedHash));
}