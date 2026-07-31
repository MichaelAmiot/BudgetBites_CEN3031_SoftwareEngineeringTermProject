#pragma once

#include <string>
#include <vector>

// Holds one user's info.
struct UserAccount {
    std::string username;
    std::string passwordHash;
    std::string passwordSalt;
    std::string profileImagePath;
};

// Keeps track of all the user accounts in memory and can save/load
// them to a file so they aren't lost when the program closes.
class UserRepository {
public:
    // returns a pointer to the account with this username, or nullptr
    // if there isn't one. Not case sensitive.
    UserAccount* find(const std::string& username);

    void add(const UserAccount& account);

    int size() const;

    // writes all the accounts out to a simple text file
    bool saveToFile(const std::string& filePath) const;

    // reads accounts back in from a file made by saveToFile
    bool loadFromFile(const std::string& filePath);

private:
    std::vector<UserAccount> accounts;
};