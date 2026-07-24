#pragma once

#include <string>

// Simple password hashing helper functions.
// This is NOT real security (a real app would use bcrypt or something)
// but shows the idea of not storing plain text passwords.

namespace PasswordSecurity {

    const int MIN_PASSWORD_LENGTH = 8;

    // returns true if password meets our requirements
    bool isStrong(const std::string& password);

    // makes a random salt string to attach to the password before hashing
    std::string generateSalt();

    // combines password + salt and runs it through our hash function
    std::string hashPassword(const std::string& password, const std::string& salt);

    // checks if the password entered matches the stored hash
    bool checkPassword(const std::string& password, const std::string& salt, const std::string& storedHash);

}