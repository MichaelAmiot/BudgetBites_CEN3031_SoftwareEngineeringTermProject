#pragma once

#include <optional>
#include <string>
#include <vector>

// A single user account record. Deliberately a plain data holder: the
// *rules* about accounts (uniqueness, password policy, who can sign in)
// belong to UX, not here -- this struct only describes what an account
// is made of.
struct UserAccount {
    std::string username;
    std::string passwordHash;      // hex-encoded SHA-256(password + salt)
    std::string passwordSalt;      // hex-encoded random salt, unique per user
    std::string profileImagePath;  // path to the stored profile image, if any
};

// -----------------------------------------------------------------------
// UserRepository
//
// Owns the in-memory collection of user accounts and knows how to
// persist them to / load them from disk. This is the *only* place in the
// program that knows the on-disk file format -- if that ever changes
// (say, to JSON or a real database), this is the only file that needs
// to change. Everything else (UX) just asks the repository to find, add,
// save, or load accounts, without caring how any of that is implemented.
// This mirrors the "separate the what from the how" principle directly.
// -----------------------------------------------------------------------
class UserRepository {
public:
    // Looks up an account by username. Comparison is case-insensitive,
    // so "Alice" and "alice" refer to the same account -- this keeps
    // usernames truly unique regardless of how they're capitalized.
    UserAccount* find(const std::string& username);
    const UserAccount* find(const std::string& username) const;

    // Adds a new account. The repository does not enforce uniqueness or
    // password policy -- those are business rules that belong to the
    // caller (UX), not to storage.
    void add(const UserAccount& account);

    // Returns how many accounts are currently held.
    std::size_t size() const;

    // Persists all accounts to 'filePath'. Only password hashes and
    // salts are written -- never a raw password, since none is ever
    // stored anywhere, even in memory. Returns false on I/O failure.
    //
    // NOTE: this file still contains password hashes and salts. Treat it
    // like any other credentials store: restrict its file permissions
    // and never commit it to source control.
    bool saveToFile(const std::string& filePath) const;

    // Loads accounts previously written by saveToFile, replacing
    // whatever is currently in memory. Returns false if the file doesn't
    // exist yet or is malformed/truncated.
    bool loadFromFile(const std::string& filePath);

private:
    std::vector<UserAccount> accounts_;
};
