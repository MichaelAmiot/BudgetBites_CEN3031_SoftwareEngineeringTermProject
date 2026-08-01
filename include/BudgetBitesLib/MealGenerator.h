//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//
#pragma once
#ifndef BUDGETBITES_MEALGENERATOR_H
#define BUDGETBITES_MEALGENERATOR_H

#include <string>
#include <vector>

#include "MealPlan.h"

class Account;
class Ingredients;
class Preferences;
class RecipeDataBase;

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

};


#endif //BUDGETBITES_MEALGENERATOR_H
