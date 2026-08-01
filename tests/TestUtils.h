#pragma once
#include <filesystem>
#include <random>
#include <sstream>

// Helper: generates a unique, unpredictable subdirectory name under the OS temp dir
inline std::filesystem::path makeUniqueTestDir(const std::string& prefix) {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss;
    oss << prefix << "-" << dist(gen);

    auto dir = std::filesystem::temp_directory_path() / oss.str();
    std::filesystem::create_directory(dir);

    std::filesystem::permissions(dir,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace);

    return dir;
}