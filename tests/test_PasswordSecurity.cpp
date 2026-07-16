#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/PasswordSecurity.h"

using PasswordSecurity::isStrong;
using PasswordSecurity::generateSalt;
using PasswordSecurity::hash;
using PasswordSecurity::verify;

TEST_CASE("isStrong rejects passwords missing a required character class", "[PasswordSecurity]") {
    CHECK_FALSE(isStrong("Ab1!"));           // too short (< kMinPasswordLength)
    CHECK_FALSE(isStrong("lowercase1!"));    // no uppercase letter
    CHECK_FALSE(isStrong("UPPERCASE1!"));    // no lowercase letter
    CHECK_FALSE(isStrong("NoDigitsHere!"));  // no digit
    CHECK_FALSE(isStrong("NoSpecial123"));   // no special/punctuation character
}

TEST_CASE("isStrong accepts a password containing every required character class", "[PasswordSecurity]") {
    REQUIRE(isStrong("Str0ng!Pass"));
}

TEST_CASE("generateSalt returns a hex string of the requested byte length", "[PasswordSecurity]") {
    // Each byte is encoded as two hex characters.
    CHECK(generateSalt(16).size() == 32);
    CHECK(generateSalt(4).size() == 8);
}

TEST_CASE("generateSalt produces different values on successive calls", "[PasswordSecurity]") {
    // Not a strict guarantee for any RNG, but with 16 random bytes a
    // collision is astronomically unlikely, so this is a safe check
    // that salts aren't accidentally constant or reused.
    REQUIRE(generateSalt() != generateSalt());
}

TEST_CASE("hash is deterministic for the same password and salt", "[PasswordSecurity]") {
    const std::string salt = generateSalt();
    REQUIRE(hash("correct-horse-battery-staple", salt) == hash("correct-horse-battery-staple", salt));
}

TEST_CASE("hash changes when the salt changes", "[PasswordSecurity]") {
    const std::string password = "correct-horse-battery-staple";
    CHECK(hash(password, "aaaa") != hash(password, "bbbb"));
}

TEST_CASE("hash changes when the password changes", "[PasswordSecurity]") {
    const std::string salt = generateSalt();
    CHECK(hash("password-one", salt) != hash("password-two", salt));
}

TEST_CASE("verify succeeds only for the exact password used to create the hash", "[PasswordSecurity]") {
    const std::string salt = generateSalt();
    const std::string storedHash = hash("MyReal Password1!", salt);

    CHECK(verify("MyReal Password1!", salt, storedHash));
    CHECK_FALSE(verify("wrong-password", salt, storedHash));
}

TEST_CASE("verify fails if the stored hash has been tampered with", "[PasswordSecurity]") {
    const std::string salt = generateSalt();
    std::string storedHash = hash("MyReal Password1!", salt);
    storedHash.back() = (storedHash.back() == '0') ? '1' : '0';

    CHECK_FALSE(verify("MyReal Password1!", salt, storedHash));
}

TEST_CASE("verify fails safely when the expected hash has a different length", "[PasswordSecurity]") {
    // Exercises constantTimeEquals's early-exit-on-length-mismatch path,
    // which must not throw or read out of bounds.
    CHECK_FALSE(verify("any-password", generateSalt(), "short"));
}
