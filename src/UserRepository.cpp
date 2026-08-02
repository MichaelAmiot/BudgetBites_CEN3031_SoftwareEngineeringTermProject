#include "BudgetBitesLib/UserRepository.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>

using namespace std;

namespace {

// lowercases a copy of the string so usernames can be compared without caring about capitalization
string toLower(const string& s) {
    string result = s;
    for (int i = 0; i < result.length(); i++) {
        result[i] = tolower(result[i]);
    }
    return result;
}

}

string UserRepository::defaultStoragePath() {
    return "data/local/users.csv";
}

UserAccount* UserRepository::find(const string& username) {
    string target = toLower(username);
    for (int i = 0; i < accounts.size(); i++) {
        if (toLower(accounts[i].username) == target) {
            return &accounts[i];
        }
    }
    return nullptr;
}

bool UserRepository::exists(const string& username) const {
    string target = toLower(username);
    for (int i = 0; i < accounts.size(); i++) {
        if (toLower(accounts[i].username) == target) {
            return true;
        }
    }
    return false;
}

void UserRepository::add(const UserAccount& account) {
    accounts.push_back(account);
}

int UserRepository::size() const {
    return accounts.size();
}

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
        // is optional (empty string) so 3 fields is valid when no image was stored
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
