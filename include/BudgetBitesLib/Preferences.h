//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//

#pragma once

#ifndef BUDGETBITES_PREFERENCES_H
#define BUDGETBITES_PREFERENCES_H

#include "BudgetBitesLib/RecipeDataBase.h"

#include <string>
#include <vector>

class Preferences {

public:
    // Legacy free-text functions kept for compatibility with existing code.
    bool addPreference(const std::string& preference);

    const std::vector<std::string>& getPreferences() const;

    // Displays all catalog tags and lets the user select by dietary tag ID.
    void enterDietaryPreferences();
    void enterDietaryPreferences(const std::vector<DietaryTag>& availableTags);

    // Displays selected tag names from the shared catalog.
    void displayDietaryPreferences() const;
    void displayDietaryPreferences(const std::vector<DietaryTag>& availableTags) const;

    bool addDietaryTagId(int dietaryTagId);
    void setDietaryTagIds(const std::vector<int>& dietaryTagIds);
    const std::vector<int>& getDietaryTagIds() const;

    // The weekly budget cannot be negative.
    bool setBudget(double budget);
    double getBudget() const;

private:
    // Kept only for compatibility with the original command-line feature.
    std::vector<std::string> preferences_;

    // Current selections use catalog IDs for filtering and persistence.
    std::vector<int> dietaryTagIds_;
    double weeklyBudget_ = 0.0;

};


#endif //BUDGETBITES_PREFERENCES_H
