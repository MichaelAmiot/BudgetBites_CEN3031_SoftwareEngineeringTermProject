//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//
#pragma once
#ifndef BUDGETBITES_MEALGENERATOR_H
#define BUDGETBITES_MEALGENERATOR_H

#include <cstddef>
#include <string>
#include <vector>

#include "MealPlan.h"

class Account;
class Ingredients;
class Preferences;
class RecipeDataBase;

enum class MealGenerationMode {
    Normal,
    BudgetFirst,
    StrictBudget
};

struct MealGenerationResult {
    bool generated = false;
    bool complete = false;
    bool withinBudget = false;
    std::size_t mealsGenerated = 0;
    double estimatedCost = 0.0;
};

class MealGenerator {

public:
    // Legacy name-based generator kept while Main is being migrated.
    void generateWeeklyMealPlan(
        MealPlan& mealPlan,
        const std::vector<std::string>& recipes
    );

    // Legacy ID-only generator kept for existing callers and tests.
    void generateWeeklyMealPlan(
        MealPlan& mealPlan,
        const std::vector<int>& recipeIds
    );

    // Generates a compatible weekly plan using budget, pantry, and reuse scoring.
    bool generateWeeklyMealPlan(
        MealPlan& mealPlan,
        const RecipeDataBase& catalog,
        const Account& account,
        const Preferences& preferences,
        const Ingredients& pantry
    );

    // Generates a plan using the selected balance of variety, cost, and completeness.
    MealGenerationResult generateWeeklyMealPlan(
        MealPlan& mealPlan,
        const RecipeDataBase& catalog,
        const Account& account,
        const Preferences& preferences,
        const Ingredients& pantry,
        MealGenerationMode mode
    );

};


#endif //BUDGETBITES_MEALGENERATOR_H
