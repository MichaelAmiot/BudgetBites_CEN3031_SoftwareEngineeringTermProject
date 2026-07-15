#pragma once

#include <optional>
#include <string>

// -----------------------------------------------------------------------
// ProfileImageStore
//
// Owns exactly one concern: validating and storing an uploaded profile
// image on disk. Kept separate from UX so that image-storage details --
// which extensions are allowed, where files live, how they're named --
// can change without touching account or sign-in logic at all.
// -----------------------------------------------------------------------
namespace ProfileImageStore {

// Validates that 'sourceImagePath' exists and has a supported image
// extension (.png, .jpg, .jpeg, .bmp, .gif), then copies it into a local
// "profile_images/" folder under a name derived from 'username'.
// Returns the new stored path on success, or std::nullopt if the source
// file doesn't exist, isn't a supported image type, or couldn't be
// copied. This is the input-validation step for image uploads: reject
// anything that doesn't conform to the expected format, rather than
// letting garbage into storage.
std::optional<std::string> store(const std::string& username, const std::string& sourceImagePath);

} // namespace ProfileImageStore
