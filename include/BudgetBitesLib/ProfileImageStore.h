#pragma once

#include <optional>
#include <string>

// Handles saving a user's uploaded profile picture to disk.

namespace ProfileImageStore {

    // checks that sourceImagePath exists and is a supported image type,
    // then copies it into data/local/profile_images/ using the username.
    // returns the new path, or nothing if it failed.
    std::optional<std::string> store(const std::string& username, const std::string& sourceImagePath);

}
