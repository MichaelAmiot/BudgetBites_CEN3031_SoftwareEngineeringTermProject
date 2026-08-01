#include "BudgetBitesLib/ProfileImageStore.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

namespace ProfileImageStore {
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
}
