//
// Created by micha on 6/17/2026.
//
//Kezia Saint-Hilaire Sprint 1
//

#pragma once

#include "BudgetBitesLib/RecipeDataBase.h"

#include <string>
#include <vector>
#ifndef ACCOUNT_H
#define ACCOUNT_H


class Account {
private:
    // Free-text values.
    std::vector<std::string> allergies;

    // Current selections use catalog IDs for filtering and persistence.
    std::vector<int> allergenIds_;

public:
    // Displays all catalog allergens and lets the user select by ID.
    void enterFoodAllergies();

    // Uses a supplied list so UX and tests can reuse the selection logic.
    void enterFoodAllergies(const std::vector<Allergen>& availableAllergens);


    // Prints selected allergen names from the shared catalog.
    void displayFoodAllergies() const;

    // Displays selected names using a supplied catalog list.
    void displayFoodAllergies(const std::vector<Allergen>& availableAllergens) const;


    std::vector<std::string> getAllergies() const;


    //Used when loading allergies back in from a file, so the loaded
    //list replaces whatever is currently stored
    void setAllergies(const std::vector<std::string>& loadedAllergies);

    // Replaces the user's selected allergens with IDs from allergens.csv.
    void setAllergenIds(const std::vector<int>& allergenIds);

    // Returns catalog IDs for UserInfoRepository and recipe filtering.
    const std::vector<int>& getAllergenIds() const;
};


#endif //ACCOUNT_H
