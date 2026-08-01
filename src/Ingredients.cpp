//
// Created by Kezia Saint-Hilaire on 7/30/2026.
//

#include "BudgetBitesLib/Ingredients.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

namespace {

std::string compactName(const std::string& name) {
    constexpr std::size_t maximumLength = 25;
    return name.size() <= maximumLength ? name : name.substr(0, maximumLength - 3) + "...";
}

void displayAvailableIngredients(const std::vector<Ingredient>& availableIngredients) {
    constexpr int columnWidth = 34;
    constexpr std::size_t columns = 3;
    for (std::size_t index = 0; index < availableIngredients.size(); ++index) {
        const Ingredient& ingredient = availableIngredients[index];
        const std::string label = std::to_string(ingredient.ingredientId) + ": " + compactName(ingredient.name);
        std::cout << std::left << std::setw(columnWidth) << label;
        if ((index + 1) % columns == 0 || index + 1 == availableIngredients.size()) {
            std::cout << '\n';
        }
    }
    std::cout << std::right;
}

} // namespace

bool Ingredients::addIngredient(const std::string& ingredient) {
    if (ingredient.empty() || hasIngredient(ingredient)) {
        return false;
    }
    ingredients_.push_back(ingredient);
    pantryItems_.clear();
    return true;
}

bool Ingredients::removeIngredient(const std::string& ingredient) {
    const auto location = std::find(ingredients_.begin(), ingredients_.end(), ingredient);
    if (location == ingredients_.end()) {
        return false;
    }
    ingredients_.erase(location);
    return true;
}

bool Ingredients::hasIngredient(const std::string& ingredient) const {
    return std::find(ingredients_.begin(), ingredients_.end(), ingredient) != ingredients_.end();
}

bool Ingredients::addIngredient(int ingredientId, std::optional<double> availableGrams) {
    if (ingredientId <= 0 || (availableGrams && *availableGrams < 0.0) || hasIngredient(ingredientId)) {
        return false;
    }
    pantryItems_.push_back({ingredientId, availableGrams});
    ingredients_.clear();
    return true;
}

bool Ingredients::removeIngredient(int ingredientId) {
    const auto location = std::find_if(pantryItems_.begin(), pantryItems_.end(), [&](const PantryItem& item) {
        return item.ingredientId == ingredientId;
    });
    if (location == pantryItems_.end()) {
        return false;
    }
    pantryItems_.erase(location);
    return true;
}

bool Ingredients::hasIngredient(int ingredientId) const {
    return std::any_of(pantryItems_.begin(), pantryItems_.end(), [&](const PantryItem& item) {
        return item.ingredientId == ingredientId;
    });
}

void Ingredients::setPantryItems(const std::vector<PantryItem>& pantryItems) {
    pantryItems_.clear();
    ingredients_.clear();
    for (const PantryItem& item : pantryItems) {
        addIngredient(item.ingredientId, item.availableGrams);
    }
}

const std::vector<PantryItem>& Ingredients::getPantryItems() const {
    return pantryItems_;
}

void Ingredients::enterIngredients() {
    RecipeDataBase catalog;
    if (!catalog.isLoaded()) {
        std::cout << "Unable to load ingredient options.\n";
        return;
    }
    enterIngredients(catalog.getAllIngredients());
}

void Ingredients::enterIngredients(const std::vector<Ingredient>& availableIngredients) {
    pantryItems_.clear();
    ingredients_.clear();

    std::cout << "\nAvailable Ingredients (ID: name):\n";
    displayAvailableIngredients(availableIngredients);
    std::cout << "Enter ingredient IDs one at a time, or 0 when finished.\n";

    int selectedId;
    while (std::cout << "Ingredient ID: " && std::cin >> selectedId) {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (selectedId == 0) {
            break;
        }

        const auto found = std::find_if(availableIngredients.begin(), availableIngredients.end(), [&](const Ingredient& ingredient) {
            return ingredient.ingredientId == selectedId;
        });
        if (found == availableIngredients.end()) {
            std::cout << "That ingredient ID is not in the list.\n";
            continue;
        }
        if (hasIngredient(selectedId)) {
            std::cout << "That ingredient is already in your pantry.\n";
            continue;
        }

        std::cout << "Available grams for " << found->name << " (press Enter to skip): ";
        std::string gramsInput;
        std::getline(std::cin, gramsInput);
        std::optional<double> grams;
        if (!gramsInput.empty()) {
            try {
                std::size_t used = 0;
                const double value = std::stod(gramsInput, &used);
                if (used != gramsInput.size() || value < 0.0) {
                    std::cout << "Please enter a non-negative number.\n";
                    continue;
                }
                grams = value;
            } catch (...) {
                std::cout << "Please enter a valid number.\n";
                continue;
            }
        }
        addIngredient(selectedId, grams);
    }
    std::cout << "\nPantry ingredients saved!\n";
}

void Ingredients::displayIngredients() const {
    RecipeDataBase catalog;
    if (!catalog.isLoaded()) {
        std::cout << "Unable to load ingredient names.\n";
        return;
    }
    displayIngredients(catalog.getAllIngredients());
}

void Ingredients::displayIngredients(const std::vector<Ingredient>& availableIngredients) const {
    std::cout << "\nPantry Ingredients:\n";
    if (pantryItems_.empty() && ingredients_.empty()) {
        std::cout << "No ingredients entered.\n";
        return;
    }

    // Current data uses IDs; old text values remain as a temporary fallback.
    if (!pantryItems_.empty()) {
        for (const PantryItem& item : pantryItems_) {
            const auto found = std::find_if(availableIngredients.begin(), availableIngredients.end(), [&](const Ingredient& ingredient) {
                return ingredient.ingredientId == item.ingredientId;
            });
            if (found != availableIngredients.end()) {
                std::cout << "- " << found->name;
                if (item.availableGrams) {
                    std::cout << " (" << *item.availableGrams << " g)";
                }
                std::cout << '\n';
            }
        }
        return;
    }
    for (const std::string& ingredient : ingredients_) {
        std::cout << "- " << ingredient << '\n';
    }
}

std::size_t Ingredients::countIngredients() const {
    return pantryItems_.empty() ? ingredients_.size() : pantryItems_.size();
}

void Ingredients::clearIngredients() {
    ingredients_.clear();
    pantryItems_.clear();
}
