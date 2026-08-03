#include "BudgetBitesLib/PasswordSecurity.h"
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <random>

using namespace std;

namespace PasswordSecurity {

    // Checks the password against our rules: long enough, and has at least
    //  one uppercase letter, one lowercase letter, one number, and one symbol.
    // Returns false the moment any rule fails.
    bool isStrong(const string& password) {
        if (password.length() < MIN_PASSWORD_LENGTH) {
            return false;
        }

        bool hasUpper = false;
        bool hasLower = false;
        bool hasNumber = false;
        bool hasSymbol = false;

        for (int i = 0; i < password.length(); i++) {
            char c = password[i];
            if (isupper(c)) {
                hasUpper = true;
            } else if (islower(c)) {
                hasLower = true;
            } else if (isdigit(c)) {
                hasNumber = true;
            } else if (ispunct(c)) {
                hasSymbol = true;
            }
        }

        return hasUpper && hasLower && hasNumber && hasSymbol;
    }


    // Builds a random 8 character string to mix into a password before hashing it.
    // Every account gets its own salt so two people with the same password
    //  still end up with completely different stored hashes.
    string generateSalt() {
        static std::random_device rd;  // pulls randomness from the OS itself

        string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        string salt = "";

        for (int i = 0; i < 8; i++) {
            salt += chars[rd() % chars.length()];
        }

        return salt;
    }

    // Turns a string into a single number using the djb2 algorithm.
    // This is not real cryptography, it just scrambles the characters
    //  enough that you cannot easily tell what the original password was.
    unsigned long simpleHash(const string& input) {
        unsigned long hash = 5381;
        for (int i = 0; i < input.length(); i++) {
            hash = ((hash << 5) + hash) + input[i]; // same as hash times 33, plus the character
        }
        return hash;
    }

    // Sticks the salt onto the end of the password, hashes the result, and
    //  turns that number into a string so it is easy to store in the CSV file and compare later on.
    string hashPassword(const string& password, const string& salt) {
        string combined = password + salt;
        unsigned long hashValue = simpleHash(combined);

        stringstream ss;
        ss << hashValue;
        return ss.str();
    }

    // Hashes the password someone just typed in using the account's stored salt,
    //  then checks if that matches the hash we already have saved for them.
    // We never store or compare raw passwords, only hashes.
    bool checkPassword(const string& password, const string& salt, const string& storedHash) {
        string newHash = hashPassword(password, salt);
        return newHash == storedHash;
    }

}