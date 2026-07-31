#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace CsvFile {

// Reads and writes CSV tables, including quoted text fields.
struct Table {
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
};

// Returns false and fills error when the file cannot be read.
bool read(const std::filesystem::path& path, Table& table, std::string& error);
bool write(
    const std::filesystem::path& path,
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows,
    std::string& error
);

} // namespace CsvFile
