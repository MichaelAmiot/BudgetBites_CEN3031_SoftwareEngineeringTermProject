#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "BudgetBitesLib/ProfileImageStore.h"

using namespace std;
namespace fs = std::filesystem;

TEST_CASE("store rejects a file that doesn't exist", "[ProfileImageStore]") {
    auto result = ProfileImageStore::store("nobody", "missing_file.png");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("store rejects an unsupported file extension", "[ProfileImageStore]") {
    string path = "upload_test.txt";
    fs::remove(path);

    ofstream out(path);
    out << "not an image";
    out.close();

    auto result = ProfileImageStore::store("someone", path);
    CHECK_FALSE(result.has_value());

    fs::remove(path);
}

TEST_CASE("store accepts a supported extension and copies the file", "[ProfileImageStore]") {
    string path = "upload_test.png";
    fs::remove(path);

    ofstream out(path);
    out << "pretend-png-bytes";
    out.close();

    auto result = ProfileImageStore::store("test_user_store", path);
    REQUIRE(result.has_value());
    CHECK(fs::exists(*result));

    ifstream in(*result);
    string contents((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    CHECK(contents == "pretend-png-bytes");

    fs::remove(path);
    fs::remove(*result);
}

TEST_CASE("store doesn't care if the extension is uppercase", "[ProfileImageStore]") {
    string path = "upload_test_upper.PNG";
    fs::remove(path);

    ofstream out(path);
    out << "pretend-png-bytes";
    out.close();

    auto result = ProfileImageStore::store("test_user_upper", path);
    REQUIRE(result.has_value());
    CHECK(fs::exists(*result));

    fs::remove(path);
    fs::remove(*result);
}

TEST_CASE("store overwrites the old picture when you upload a new one", "[ProfileImageStore]") {
    string path1 = "upload_first.png";
    string path2 = "upload_second.png";
    fs::remove(path1);
    fs::remove(path2);

    ofstream out1(path1);
    out1 << "version-one";
    out1.close();
    auto result1 = ProfileImageStore::store("test_user_overwrite", path1);
    REQUIRE(result1.has_value());

    ofstream out2(path2);
    out2 << "version-two";
    out2.close();
    auto result2 = ProfileImageStore::store("test_user_overwrite", path2);
    REQUIRE(result2.has_value());

    CHECK(*result1 == *result2); // same username -> same destination file

    ifstream in(*result2);
    string contents((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    CHECK(contents == "version-two");

    fs::remove(path1);
    fs::remove(path2);
    fs::remove(*result2);
}
