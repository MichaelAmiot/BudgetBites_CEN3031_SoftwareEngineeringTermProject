#include "BudgetBitesLib/UserRepository.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>

using namespace std;

namespace {

// Makes a lowercase copy of a string so two usernames can be compared
//  without caring whether someone typed capital letters or not.
string toLower(const string& s) {
    string result = s;
    for (int i = 0; i < result.length(); i++) {
        result[i] = tolower(result[i]);
    }
    return result;
}

}

// Where account data gets saved and loaded from when the caller does not
//  pass their own file path.
string UserRepository::defaultStoragePath() {
    return "data/local/users.csv";
}

// Looks through every loaded account for one with a matching username.
// The comparison is not case sensitive, so "Mike" and "mike" are treated
//  as the same person. Returns a pointer to the real account so the caller
//  can update it, or nullptr if nobody has that username yet.
UserAccount* UserRepository::find(const string& username) {
    string target = toLower(username);
    for (int i = 0; i < accounts.size(); i++) {
        if (toLower(accounts[i].username) == target) {
            return &accounts[i];
        }
    }
    return nullptr;
}

// Same idea as find, but this one just answers yes or no. Handy for
//  checking whether a username is available before anyone has typed a password yet.
bool UserRepository::exists(const string& username) const {
    string target = toLower(username);
    for (int i = 0; i < accounts.size(); i++) {
        if (toLower(accounts[i].username) == target) {
            return true;
        }
    }
    return false;
}

// Just tacks a new account onto the end of the list. Whoever calls this
//  is responsible for making sure the username is not already taken.
void UserRepository::add(const UserAccount& account) {
    accounts.push_back(account);
}

// How many accounts are currently loaded in memory.
int UserRepository::size() const {
    return accounts.size();
}

// Writes every account out to a plain CSV file, one account per line.
// Returns false if the file could not be opened for writing.
bool UserRepository::saveToFile(const string& filePath) const {
    ofstream out(filePath);
    if (!out) {
        return false;
    }

    // one account per line, fields separated by commas
    for (int i = 0; i < accounts.size(); i++) {
        out << accounts[i].username << ","
            << accounts[i].passwordHash << ","
            << accounts[i].passwordSalt << ","
            << accounts[i].profileImagePath << "\n";
    }

    return true;
}

// Reads accounts back in from a CSV file made by saveToFile. Missing the
//  file is treated as a normal case, since that just means nobody has
//  registered yet, so it still returns true. Any line that does not have
//  enough fields gets skipped instead of crashing the whole load.
bool UserRepository::loadFromFile(const string& filePath) {
    ifstream in(filePath);
    if (!in) {
        return false; // no file yet, that's fine on first run
    }

    vector<UserAccount> loaded;
    string line;

    while (getline(in, line)) {
        stringstream ss(line);
        string field;
        vector<string> fields;

        while (getline(ss, field, ',')) {
            fields.push_back(field);
        }

        // skip lines that don't have the minimum required fields; profileImagePath
        //  is optional (empty string) so 3 fields is valid when no image was stored.
        if (fields.size() < 3) {
            continue;
        }

        UserAccount account;
        account.username = fields[0];
        account.passwordHash = fields[1];
        account.passwordSalt = fields[2];
        account.profileImagePath = (fields.size() >= 4) ? fields[3] : "";
        loaded.push_back(account);
    }

    accounts = loaded;
    return true;
}
