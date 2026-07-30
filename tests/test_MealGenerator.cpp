//
// Created by Kezia Saint-Hilaire on 7/29/2026.
#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/MealGenerator.h"

TEST_CASE("Weekly meal plan can be generated", "[MealGenerator]") {



    MealPlan mealPlan;
    MealGenerator generator;

    std::vector<std::string> recipes = {



        "Oatmeal",
        "Chicken Salad",
        "Pasta"
    };

    generator.generateWeeklyMealPlan(
        mealPlan,
        recipes
    );

    CHECK(mealPlan.countMeals() == 21);

    CHECK(mealPlan.isComplete());
}

TEST_CASE("Generator adds recipes to meal plan", "[MealGenerator]") {
    MealPlan mealPlan;
    MealGenerator generator;



    std::vector<std::string> recipes = {
        "Oatmeal",
        "Salad"
    };

    generator.generateWeeklyMealPlan(
        mealPlan,
        recipes
    );

    const MealEntry* breakfast =

        mealPlan.getMeal(0, MealType::Breakfast);

    REQUIRE(breakfast != nullptr);


    CHECK(breakfast->recipeName == "Oatmeal");


    CHECK(breakfast->estimatedCost == 5.00);
}