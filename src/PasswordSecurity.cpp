#include "BudgetBitesLib/PasswordSecurity.h"
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <random>

using namespace std;

namespace PasswordSecurity {

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


    string generateSalt() {
        static std::random_device rd;  // CSPRNG-backed (OS entropy source)

        string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        string salt = "";

        for (int i = 0; i < 8; i++) {
            salt += chars[rd() % chars.length()];
        }

        return salt;
    }

    // basic string hash function (not real cryptography, just spreads
    // the characters out so it's not obvious what the password is)
    // based on the djb2 hash algorithm
    unsigned long simpleHash(const string& input) {
        unsigned long hash = 5381;
        for (int i = 0; i < input.length(); i++) {
            hash = ((hash << 5) + hash) + input[i]; // hash * 33 + c
        }
        return hash;
    }

    string hashPassword(const string& password, const string& salt) {
        string combined = password + salt;
        unsigned long hashValue = simpleHash(combined);

        // convert the number to a string so it can be stored/compared easily
        stringstream ss;
        ss << hashValue;
        return ss.str();
    }

    bool checkPassword(const string& password, const string& salt, const string& storedHash) {
        string newHash = hashPassword(password, salt);
        return newHash == storedHash;
    }

}