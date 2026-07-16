#include "BudgetBitesLib/ProfileImageStore.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace ProfileImageStore {

std::optional<std::string> store(const std::string& username, const std::string& sourceImagePath) {
    fs::path source(sourceImagePath);
    if (!fs::exists(source) || !fs::is_regular_file(source)) {
        return std::nullopt;
    }

    static const std::vector<std::string> kAllowedExtensions = {
        ".png", ".jpg", ".jpeg", ".bmp", ".gif"
    };
    std::string ext = source.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (std::find(kAllowedExtensions.begin(), kAllowedExtensions.end(), ext) == kAllowedExtensions.end()) {
        return std::nullopt;
    }

    fs::path destDir("profile_images");
    std::error_code ec;
    fs::create_directories(destDir, ec);
    if (ec) {
        return std::nullopt;
    }

    fs::path destPath = destDir / (username + ext);

    // Remove any previously stored image for this user before copying,
    // rather than relying solely on copy_options::overwrite_existing --
    // that flag is not reliably honored by every standard library
    // implementation when the destination file already exists.
    fs::remove(destPath, ec);
    ec.clear();

    fs::copy_file(source, destPath, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        return std::nullopt;
    }

    return destPath.string();
}

} // namespace ProfileImageStore
