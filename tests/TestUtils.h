#pragma once

#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

inline std::filesystem::path makeUniqueTestDir(const std::string& prefix) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss;
    oss << prefix << "-" << std::hex << dist(gen);

    return std::filesystem::temp_directory_path() / oss.str();
}
