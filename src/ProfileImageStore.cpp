#include "BudgetBitesLib/ProfileImageStore.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

namespace ProfileImageStore {
    // Copies a profile picture into local storage so BudgetBites has its own
    //  copy tied to that username, instead of relying on the original file
    //  staying in place. Only a handful of common image extensions are
    //  allowed, and the function returns an empty result if the source file
    //  is missing, has the wrong extension, or cannot be copied for any reason.
    optional<string> store(const string& username, const string& sourceImagePath) {
        fs::path source(sourceImagePath);

        if (!fs::exists(source)) {
            return nullopt;
        }

        // only allow certain extensions
        vector<string> allowedExtensions = {".png", ".jpg", ".jpeg", ".bmp", ".gif"};

        string ext = source.extension().string();
        for (int i = 0; i < ext.length(); i++) {
            ext[i] = tolower(ext[i]);
        }

        bool isAllowed = false;
        for (int i = 0; i < allowedExtensions.size(); i++) {
            if (ext == allowedExtensions[i]) {
                isAllowed = true;
                break;
            }
        }
        if (!isAllowed) {
            return nullopt;
        }

        // make sure the destination folder exists
        const fs::path destinationDirectory =
                fs::path("data") / "local" / "profile_images";
        fs::create_directories(destinationDirectory);

        fs::path destPath = destinationDirectory / (username + ext);

        // copy the bytes over manually
        ifstream in(source, ios::binary);
        ofstream out(destPath, ios::binary | ios::trunc);
        if (!in || !out) {
            return nullopt;
        }
        out << in.rdbuf();

        return destPath.string();
    }

    bool remove(const string& imagePath) {
        if (imagePath.empty()) {
            return true;
        }

        std::error_code errorCode;
        fs::remove(fs::path(imagePath), errorCode);

        // fs::remove sets errorCode when something goes wrong, but a missing
        //  file is not treated as an error here since the goal (no file at that path) is already true.
        return !errorCode || !fs::exists(imagePath);
    }
}
