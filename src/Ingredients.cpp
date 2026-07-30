//
// Created by Kezia Saint-Hilaire on 7/30/2026.
//

#include "../include/BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/Grocery.h"

#include <algorithm>


// Adds Ingredient

bool Ingredients::addIngredient(const std::string& ingredient) {
    if (ingredient.empty()) {
        return false;
    }


    if (hasIngredient(ingredient)) {
        return false;


    }



    ingredients_.push_back(ingredient);

    return true;
}



//remove ingredient

bool Ingredients::removeIngredient(const std::string& ingredient) {
    auto location =std::find(
        ingredients_.begin(), ingredients_.end(), ingredient);

    if (location == ingredients_.end()) {
        return false;

    }

    ingredients_.erase(location);


    return true;


}



bool Ingredients::hasIngredient(const std::string& ingredient) const {
    return std::find(ingredients_.begin(),

        ingredients_.end(),

        ingredient


        ) != ingredients_.end();

}


//Display the ingredients

void Ingredients::displayIngredients() const {
    std::cout << "\nAvailable Ingredients:\n";

    if (ingredients_.empty()) {
        std::cout << "No ingredients enter.\n";
        return;


    }

    for (std::size_t index=0; index < ingredients_.size(); ++index) {
        std::cout << index +1 << ". " <<ingredients_[index] << "\n";
    }
}

std::size_t Ingredients::countIngredients() const {
    return ingredients_.size();

}

//removes all the ingredients


void Ingredients::clearIngredients() {
    ingredients_.clear();
}

