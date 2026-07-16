#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "BudgetBitesLib/ProfileImageStore.h"

namespace {

namespace fs = std::filesystem;

// Creates a small file at 'path' with 'contents' so it can act as a fake
// "uploaded image" for store() to validate and copy, without needing a
// real image on disk.
void writeFile(const fs::path& path, const std::string& contents) {
    std::ofstream out(path, std::ios::binary);
    out << contents;
}

// Removes a source file and its corresponding entry (if any) under
// profile_images/ so tests don't leak files into the working directory
// or interfere with each other.
struct TempImage {
    fs::path sourcePath;
    fs::path storedPath;
    explicit TempImage(const fs::path& source) : sourcePath(source) {
        fs::remove(sourcePath);
    }
    ~TempImage() {
        fs::remove(sourcePath);
        if (!storedPath.empty()) {
            fs::remove(storedPath);
        }
    }
};

} // namespace

TEST_CASE("store() rejects a source path that doesn't exist", "[ProfileImageStore]") {
    auto result = ProfileImageStore::store("nobody", "definitely_missing_file.png");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("store() rejects a file with an unsupported extension", "[ProfileImageStore]") {
    TempImage image("upload_test.txt");
    writeFile(image.sourcePath, "not an image");

    auto result = ProfileImageStore::store("someone", image.sourcePath.string());
    CHECK_FALSE(result.has_value());
}

TEST_CASE("store() accepts a supported extension and copies the file into profile_images/", "[ProfileImageStore]") {
    TempImage image("upload_test.png");
    writeFile(image.sourcePath, "pretend-png-bytes");

    auto result = ProfileImageStore::store("test_user_store", image.sourcePath.string());
    REQUIRE(result.has_value());
    image.storedPath = *result;

    CHECK(fs::exists(image.storedPath));
    CHECK(image.storedPath.filename() == "test_user_store.png");

    std::ifstream in(image.storedPath, std::ios::binary);
    std::string copiedContents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(copiedContents == "pretend-png-bytes");
}

TEST_CASE("store() extension matching is case-insensitive", "[ProfileImageStore]") {
    TempImage image("upload_test_upper.PNG");
    writeFile(image.sourcePath, "pretend-png-bytes");

    auto result = ProfileImageStore::store("test_user_upper", image.sourcePath.string());
    REQUIRE(result.has_value());
    image.storedPath = *result;

    CHECK(fs::exists(image.storedPath));
}

TEST_CASE("store() overwrites a previously stored image for the same username", "[ProfileImageStore]") {
    TempImage firstUpload("upload_first.png");
    writeFile(firstUpload.sourcePath, "version-one");
    auto firstResult = ProfileImageStore::store("test_user_overwrite", firstUpload.sourcePath.string());
    REQUIRE(firstResult.has_value());
    firstUpload.storedPath = *firstResult;

    TempImage secondUpload("upload_second.png");
    writeFile(secondUpload.sourcePath, "version-two");
    auto secondResult = ProfileImageStore::store("test_user_overwrite", secondUpload.sourcePath.string());
    REQUIRE(secondResult.has_value());
    secondUpload.storedPath = *secondResult;

    CHECK(firstResult == secondResult); // same destination path for the same username

    std::ifstream in(*secondResult, std::ios::binary);
    std::string finalContents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(finalContents == "version-two");
}
