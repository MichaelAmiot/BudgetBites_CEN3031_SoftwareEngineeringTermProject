//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//

#include "BudgetBitesLib/Preferences.h"

#include <algorithm>
#include <iostream>

bool Preferences::addPreference(const std::string& preference) {
    if (preference.empty() ||
        std::find(preferences_.begin(), preferences_.end(), preference) != preferences_.end()) {
        return false;
    }
    preferences_.push_back(preference);
    dietaryTagIds_.clear();
    return true;
}

const std::vector<std::string>& Preferences::getPreferences() const {
    return preferences_;
}

void Preferences::enterDietaryPreferences() {
    RecipeDataBase catalog;
    if (!catalog.isLoaded()) {
        std::cout << "Unable to load dietary preference options.\n";
        return;
    }
    enterDietaryPreferences(catalog.getDietaryTags());
}

void Preferences::enterDietaryPreferences(const std::vector<DietaryTag>& availableTags) {
    dietaryTagIds_.clear();
    preferences_.clear();

    std::cout << "\nAvailable Dietary Preferences:\n";
    for (const DietaryTag& tag : availableTags) {
        std::cout << tag.dietaryTagId << ". " << tag.displayName << '\n';
    }

    std::cout << "Enter dietary tag IDs one at a time, or 0 when finished.\n";
    int selectedId;
    while (std::cout << "Dietary tag ID: " && std::cin >> selectedId && selectedId != 0) {
        const auto found = std::find_if(availableTags.begin(), availableTags.end(), [&](const DietaryTag& tag) {
            return tag.dietaryTagId == selectedId;
        });
        if (found == availableTags.end()) {
            std::cout << "That dietary tag ID is not in the list.\n";
            continue;
        }
        addDietaryTagId(selectedId);
    }
    std::cout << "\nDietary preferences saved!\n";
}

void Preferences::displayDietaryPreferences() const {
    RecipeDataBase catalog;
    if (!catalog.isLoaded()) {
        std::cout << "Unable to load dietary preference names.\n";
        return;
    }
    displayDietaryPreferences(catalog.getDietaryTags());
}

void Preferences::displayDietaryPreferences(const std::vector<DietaryTag>& availableTags) const {
    std::cout << "\nDietary Preferences:\n";
    if (dietaryTagIds_.empty() && preferences_.empty()) {
        std::cout << "None entered.\n";
        return;
    }

    // Current data uses IDs; old text values remain as a temporary fallback.
    if (!dietaryTagIds_.empty()) {
        for (const int tagId : dietaryTagIds_) {
            const auto found = std::find_if(availableTags.begin(), availableTags.end(), [&](const DietaryTag& tag) {
                return tag.dietaryTagId == tagId;
            });
            if (found != availableTags.end()) {
                std::cout << "- " << found->displayName << '\n';
            }
        }
        return;
    }
    for (const std::string& preference : preferences_) {
        std::cout << "- " << preference << '\n';
    }
}

bool Preferences::addDietaryTagId(int dietaryTagId) {
    if (dietaryTagId <= 0 ||
        std::find(dietaryTagIds_.begin(), dietaryTagIds_.end(), dietaryTagId) != dietaryTagIds_.end()) {
        return false;
    }
    dietaryTagIds_.push_back(dietaryTagId);
    preferences_.clear();
    return true;
}

void Preferences::setDietaryTagIds(const std::vector<int>& dietaryTagIds) {
    dietaryTagIds_.clear();
    preferences_.clear();
    for (const int tagId : dietaryTagIds) {
        addDietaryTagId(tagId);
    }
}

const std::vector<int>& Preferences::getDietaryTagIds() const {
    return dietaryTagIds_;
}

bool Preferences::setBudget(double budget) {
    if (budget < 0.0) {
        return false;
    }
    weeklyBudget_ = budget;
    return true;
}

double Preferences::getBudget() const {
    return weeklyBudget_;
}
