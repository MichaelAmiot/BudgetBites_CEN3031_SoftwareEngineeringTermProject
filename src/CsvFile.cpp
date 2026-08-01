#include "BudgetBitesLib/CsvFile.h"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace {

bool parse(const std::string& contents, CsvFile::Table& table, std::string& error) {
    std::vector<std::string> row;
    std::string field;
    bool inQuotes = false;

    // A quoted field may contain a line break, so rows finish only outside quotes.
    const auto finishRow = [&]() {
        row.push_back(field);
        field.clear();
        if (!row.empty() && !(row.size() == 1 && row.front().empty())) {
            table.rows.push_back(row);
        }
        row.clear();
    };

    for (std::size_t index = 0; index < contents.size(); ++index) {
        const char character = contents[index];
        if (inQuotes) {
            if (character == '"') {
                if (index + 1 < contents.size() && contents[index + 1] == '"') {
                    field += '"';
                    ++index;
                } else {
                    inQuotes = false;
                }
            } else {
                field += character;
            }
            continue;
        }

        if (character == '"') {
            if (!field.empty()) {
                error = "Unexpected quote in CSV field.";
                return false;
            }
            inQuotes = true;
        } else if (character == ',') {
            row.push_back(field);
            field.clear();
        } else if (character == '\n') {
            finishRow();
        } else if (character != '\r') {
            field += character;
        }
    }

    if (inQuotes) {
        error = "Unterminated quoted CSV field.";
        return false;
    }
    if (!field.empty() || !row.empty()) {
        finishRow();
    }
    if (table.rows.empty()) {
        error = "CSV file is empty.";
        return false;
    }

    // The first row defines the column names for every later row.
    table.headers = std::move(table.rows.front());
    table.rows.erase(table.rows.begin());
    for (const auto& dataRow : table.rows) {
        if (dataRow.size() != table.headers.size()) {
            error = "CSV row does not match the header column count.";
            return false;
        }
    }
    return true;
}

void writeField(std::ofstream& output, const std::string& value) {
    const bool quoted = value.find_first_of(",\"\n\r") != std::string::npos;
    if (quoted) {
        output << '"';
    }
    for (const char character : value) {
        if (character == '"') {
            output << '"';
        }
        output << character;
    }
    if (quoted) {
        output << '"';
    }
}

} // namespace

namespace CsvFile {

bool read(const std::filesystem::path& path, Table& table, std::string& error) {
    table = {};
    error.clear();

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Unable to open CSV file: " + path.string();
        return false;
    }
    std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (contents.size() >= 3 && static_cast<unsigned char>(contents[0]) == 0xEF &&
        static_cast<unsigned char>(contents[1]) == 0xBB && static_cast<unsigned char>(contents[2]) == 0xBF) {
        contents.erase(0, 3);
    }
    return parse(contents, table, error);
}

bool write(
    const std::filesystem::path& path,
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows,
    std::string& error
) {
    error.clear();

    for (const auto& row : rows) {
        if (row.size() != headers.size()) {
            error = "CSV row does not match the header column count.";
            return false;
        }
    }
    const auto directory = path.parent_path();
    if (!directory.empty()) {
        std::error_code directoryError;
        std::filesystem::create_directories(directory, directoryError);
        if (directoryError) {
            error = "Unable to create CSV directory: " + directory.string();
            return false;
        }
    }
    // Write a temp file first so a failed save does not damage existing data.
    const auto temporaryPath = path.string() + ".tmp";
    std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "Unable to write CSV file: " + path.string();
        return false;
    }
    const auto writeRow = [&](const std::vector<std::string>& row) {
        for (std::size_t index = 0; index < row.size(); ++index) {
            if (index != 0) {
                output << ',';
            }
            writeField(output, row[index]);
        }
        output << '\n';
    };
    writeRow(headers);
    for (const auto& row : rows) {
        writeRow(row);
    }
    output.close();
    if (!output) {
        error = "Unable to finish writing CSV file: " + path.string();
        return false;
    }
    std::error_code renameError;
    std::filesystem::rename(temporaryPath, path, renameError);
    if (renameError) {
        std::filesystem::remove(temporaryPath);
        error = "Unable to replace CSV file: " + path.string();
        return false;
    }
    return true;
}

} // namespace CsvFile
