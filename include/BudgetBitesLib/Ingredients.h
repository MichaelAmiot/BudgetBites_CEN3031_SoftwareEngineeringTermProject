//
// Created by Kezia Saint-Hilaire on 7/30/2026.
//

#ifndef BUDGETBITES_INGREDIENTS_H
#define BUDGETBITES_INGREDIENTS_H

#include "BudgetBitesLib/RecipeDataBase.h"

#include <optional>
#include <string>
#include <vector>

struct PantryItem {
    int ingredientId;
    // Empty grams means the user owns the ingredient but did not measure it.
    std::optional<double> availableGrams;
};

class Ingredients {

public:
    // Legacy free-text operations kept for compatibility with existing code.
    bool addIngredient(const std::string& ingredient);
    bool removeIngredient(const std::string& ingredient);
    bool hasIngredient(const std::string& ingredient) const;

    // Current pantry operations use ingredient IDs and optional gram amounts.
    bool addIngredient(int ingredientId, std::optional<double> availableGrams = std::nullopt);
    bool removeIngredient(int ingredientId);
    bool hasIngredient(int ingredientId) const;

    void setPantryItems(const std::vector<PantryItem>& pantryItems);
    const std::vector<PantryItem>& getPantryItems() const;

    // Displays the catalog compactly and lets the user select pantry items.
    void enterIngredients();
    void enterIngredients(const std::vector<Ingredient>& availableIngredients);

    void displayIngredients() const;
    void displayIngredients(const std::vector<Ingredient>& availableIngredients) const;

    std::size_t countIngredients() const;
    void clearIngredients();

private:
    // Kept only for compatibility with the original command-line feature.
    std::vector<std::string> ingredients_;

    // Current pantry data is ready for filtering, grocery calculations, and saving.
    std::vector<PantryItem> pantryItems_;

};


#endif //BUDGETBITES_INGREDIENTS_H#pragma once   // prevents double-inclusion


