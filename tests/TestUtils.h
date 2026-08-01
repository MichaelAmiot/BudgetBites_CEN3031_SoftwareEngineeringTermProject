#pragma once
#include <filesystem>
#include <random>
#include <sstream>

// Helper: generates a unique, unpredictable subdirectory name under the OS temp dir
inline std::filesystem::path makeUniqueTestDir(const std::string& prefix) {
    static std::random_device rd;

    std::ostringstream oss;
    oss << prefix << "-" << rd();

    auto dir = std::filesystem::temp_directory_path() / oss.str(); // NOSONAR: path uses a cryptographically unpredictable suffix and owner-only permissions are set below, mitigating symlink/collision risk
    std::filesystem::create_directory(dir);

    std::filesystem::permissions(dir,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace);

    return dir;
}