#pragma once

#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/RecipeDataBase.h"

#include <cstddef>
#include <iosfwd>
#include <vector>

class Account;
class Grocery;
class Ingredients;
class Preferences;

// Shared operations used by the CLI Main without owning terminal input.
class MainHelper {
public:
    static void resetSessionState(
        Account& account,
        Preferences& preferences,
        Ingredients& ingredients,
        MealPlan& mealPlan,
        Grocery& grocery
    );

    static void displayBudgetStatus(
        const Grocery& grocery,
        const Preferences& preferences,
        std::ostream& output
    );

    static bool displayRecipeDetails(
        const MealPlan& mealPlan,
        std::size_t dayIndex,
        MealType mealType,
        const RecipeDataBase& catalog,
        std::ostream& output
    );

    static std::vector<Recipe> compatibleRecipesForMeal(
        MealType mealType,
        const RecipeDataBase& catalog,
        const Account& account,
        const Preferences& preferences
    );

    static void displayRecipeOptions(
        const std::vector<Recipe>& recipes,
        std::ostream& output
    );

    static bool replaceMeal(
        MealPlan& mealPlan,
        Grocery& grocery,
        std::size_t dayIndex,
        MealType mealType,
        int recipeId,
        const RecipeDataBase& catalog,
        const Account& account,
        const Preferences& preferences,
        const Ingredients& ingredients
    );
};
