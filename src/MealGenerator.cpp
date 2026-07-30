//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//

#include "BudgetBitesLib/MealGenerator.h"

void MealGenerator::generateWeeklyMealPlan(MealPlan& mealPlan, const std::vector<std::string> &recipes) {

    if (recipes.empty()) {

        return;
    }

    std::size_t recipeIndex = 0;

    for (int day=0; day < 7; day++) {

        mealPlan.setMeal(
            day,
            MealType::Breakfast,
            recipes[recipeIndex],
        5.00);
        recipeIndex++;

        if (recipeIndex >= recipes.size()) {
            recipeIndex = 0;


        }

    mealPlan.setMeal(
        day,
        MealType::Lunch,
        recipes[recipeIndex],
        7.50);


        recipeIndex++;

        if (recipeIndex >= recipes.size()) {
            recipeIndex = 0;


        }

        mealPlan.setMeal(
            day,
            MealType::Dinner,
            recipes[recipeIndex],
            10.00);


        recipeIndex++;


        if (recipeIndex >= recipes.size()) {
            recipeIndex=0;
        }


    }

}
