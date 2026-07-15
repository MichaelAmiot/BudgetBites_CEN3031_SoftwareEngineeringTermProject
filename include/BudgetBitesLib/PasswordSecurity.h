#pragma once

#include <cstddef>
#include <string>

// -----------------------------------------------------------------------
// PasswordSecurity
//
// Owns exactly one concern: keeping passwords safe. Nothing in here knows
// about user accounts, sign-in flows, or file storage -- that separation
// is deliberate (Sommerville, "Separation of Concerns": each abstraction
// should address one concern, and everything about that concern should
// live in one place). If we ever swap SHA-256 for bcrypt/Argon2, this is
// the only file that changes.
//
// SECURITY NOTE: a single round of SHA-256 is fast by design, which is
// good for general hashing but not ideal for password storage, since it
// makes brute-forcing leaked hashes cheaper. For production use, prefer a
// dedicated password-hashing algorithm with a built-in work factor, such
// as bcrypt, scrypt, or Argon2 (e.g. via libsodium). This implementation
// demonstrates the core concepts -- salting, hashing, never storing raw
// passwords, constant-time comparison -- without an external dependency.
// -----------------------------------------------------------------------
namespace PasswordSecurity {

// Named constant instead of a magic number scattered through the code
// (Sommerville: "name constants rather than using absolute numbers").
inline constexpr std::size_t kMinPasswordLength = 8;

// True if 'password' has at least kMinPasswordLength characters and
// includes an uppercase letter, a lowercase letter, a digit, and a
// special character. This is the input-validation rule for passwords:
// "define the expected format for user inputs and validate that all
// inputs conform to that format."
bool isStrong(const std::string& password);

// Generates a cryptographically-random, hex-encoded salt.
std::string generateSalt(std::size_t numBytes = 16);

// Computes a hex-encoded SHA-256 hash of (password + salt).
std::string hash(const std::string& password, const std::string& salt);

// Returns true if 'password', when hashed with 'salt', matches
// 'expectedHash'. Uses a constant-time comparison internally so that
// failed sign-ins don't leak information via response timing.
bool verify(const std::string& password, const std::string& salt, const std::string& expectedHash);

} // namespace PasswordSecurity
