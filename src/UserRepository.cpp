#include "BudgetBitesLib/UserRepository.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>

namespace {

std::string toLowerCopy(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

// Writes a length-prefixed string: a 4-byte big-endian length, followed
// by that many raw bytes. This avoids any need to escape delimiters,
// regardless of what characters happen to show up in a username or path.
void writeLengthPrefixed(std::ofstream& out, const std::string& value) {
    uint32_t len = static_cast<uint32_t>(value.size());
    unsigned char lenBytes[4] = {
        static_cast<unsigned char>((len >> 24) & 0xFF),
        static_cast<unsigned char>((len >> 16) & 0xFF),
        static_cast<unsigned char>((len >> 8) & 0xFF),
        static_cast<unsigned char>(len & 0xFF)
    };
    out.write(reinterpret_cast<const char*>(lenBytes), 4);
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

// Reads back a string written by writeLengthPrefixed. Returns false if
// the stream doesn't have enough bytes left (e.g. a truncated file) --
// this is the "failure management" half of loading: a corrupt file
// should be reported, not crash the program.
bool readLengthPrefixed(std::ifstream& in, std::string& out) {
    unsigned char lenBytes[4];
    in.read(reinterpret_cast<char*>(lenBytes), 4);
    if (!in) {
        return false;
    }
    uint32_t len = (static_cast<uint32_t>(lenBytes[0]) << 24) |
                   (static_cast<uint32_t>(lenBytes[1]) << 16) |
                   (static_cast<uint32_t>(lenBytes[2]) << 8) |
                   (static_cast<uint32_t>(lenBytes[3]));

    out.resize(len);
    if (len > 0) {
        in.read(&out[0], static_cast<std::streamsize>(len));
        if (!in) {
            return false;
        }
    }
    return true;
}

constexpr char kFileMagic[4] = {'B', 'B', 'U', '1'}; // "BudgetBites Users v1"

} // namespace

UserAccount* UserRepository::find(const std::string& username) {
    const std::string target = toLowerCopy(username);
    auto it = std::find_if(accounts_.begin(), accounts_.end(),
        [&](const UserAccount& u) { return toLowerCopy(u.username) == target; });
    return it != accounts_.end() ? &(*it) : nullptr;
}

const UserAccount* UserRepository::find(const std::string& username) const {
    const std::string target = toLowerCopy(username);
    auto it = std::find_if(accounts_.begin(), accounts_.end(),
        [&](const UserAccount& u) { return toLowerCopy(u.username) == target; });
    return it != accounts_.end() ? &(*it) : nullptr;
}

void UserRepository::add(const UserAccount& account) {
    accounts_.push_back(account);
}

std::size_t UserRepository::size() const {
    return accounts_.size();
}

bool UserRepository::saveToFile(const std::string& filePath) const {
    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }

    out.write(kFileMagic, 4);

    uint32_t count = static_cast<uint32_t>(accounts_.size());
    unsigned char countBytes[4] = {
        static_cast<unsigned char>((count >> 24) & 0xFF),
        static_cast<unsigned char>((count >> 16) & 0xFF),
        static_cast<unsigned char>((count >> 8) & 0xFF),
        static_cast<unsigned char>(count & 0xFF)
    };
    out.write(reinterpret_cast<const char*>(countBytes), 4);

    for (const auto& account : accounts_) {
        writeLengthPrefixed(out, account.username);
        writeLengthPrefixed(out, account.passwordHash);
        writeLengthPrefixed(out, account.passwordSalt);
        writeLengthPrefixed(out, account.profileImagePath);
    }

    return static_cast<bool>(out);
}

bool UserRepository::loadFromFile(const std::string& filePath) {
    std::ifstream in(filePath, std::ios::binary);
    if (!in) {
        return false; // file doesn't exist yet -- not an error on first run
    }

    char magic[4];
    in.read(magic, 4);
    if (!in || std::string(magic, 4) != std::string(kFileMagic, 4)) {
        return false; // missing or unrecognized file format
    }

    unsigned char countBytes[4];
    in.read(reinterpret_cast<char*>(countBytes), 4);
    if (!in) {
        return false;
    }
    uint32_t count = (static_cast<uint32_t>(countBytes[0]) << 24) |
                      (static_cast<uint32_t>(countBytes[1]) << 16) |
                      (static_cast<uint32_t>(countBytes[2]) << 8) |
                      (static_cast<uint32_t>(countBytes[3]));

    std::vector<UserAccount> loaded;
    loaded.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        UserAccount account;
        if (!readLengthPrefixed(in, account.username) ||
            !readLengthPrefixed(in, account.passwordHash) ||
            !readLengthPrefixed(in, account.passwordSalt) ||
            !readLengthPrefixed(in, account.profileImagePath)) {
            return false; // truncated or corrupt file
        }
        loaded.push_back(std::move(account));
    }

    accounts_ = std::move(loaded);
    return true;
}
