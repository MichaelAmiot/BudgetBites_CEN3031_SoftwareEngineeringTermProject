//
// Created by Kezia Saint-Hilaire on 7/29/2026.
#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/MealGenerator.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/RecipeDataBase.h"

#include <filesystem>
#include <unordered_set>

namespace {

std::filesystem::path catalogPath() {
    return std::filesystem::path(BUDGETBITES_SOURCE_DIR) / "data" / "seed";
}

} // namespace

TEST_CASE("Weekly meal plan can be generated", "[MealGenerator]") {



    MealPlan mealPlan;
    MealGenerator generator;

    std::vector<int> recipes = {
        2,
        6,
        7
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



    std::vector<int> recipes = {
        6,
        7
    };

    generator.generateWeeklyMealPlan(
        mealPlan,
        recipes
    );

    const MealEntry* breakfast =

        mealPlan.getMeal(0, MealType::Breakfast);

    REQUIRE(breakfast != nullptr);


    REQUIRE(breakfast->recipeId.has_value());
    CHECK(*breakfast->recipeId == 6);
}

TEST_CASE("Catalog generator creates a complete plan with the required meal categories", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Account account;
    Preferences preferences;
    REQUIRE(preferences.setBudget(100000.0));
    Ingredients pantry;
    MealPlan mealPlan;
    MealGenerator generator;

    REQUIRE(generator.generateWeeklyMealPlan(mealPlan, catalog, account, preferences, pantry));
    REQUIRE(mealPlan.isComplete());

    std::unordered_set<int> firstBreakfastRecipes;
    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        const MealEntry* breakfast = mealPlan.getMeal(day, MealType::Breakfast);
        const MealEntry* lunch = mealPlan.getMeal(day, MealType::Lunch);
        const MealEntry* dinner = mealPlan.getMeal(day, MealType::Dinner);
        REQUIRE(breakfast != nullptr);
        REQUIRE(breakfast->recipeId.has_value());
        REQUIRE(lunch != nullptr);
        REQUIRE(lunch->recipeId.has_value());
        REQUIRE(dinner != nullptr);
        REQUIRE(dinner->recipeId.has_value());

        const auto breakfastRecipe = catalog.getRecipeById(*breakfast->recipeId);
        const auto lunchRecipe = catalog.getRecipeById(*lunch->recipeId);
        const auto dinnerRecipe = catalog.getRecipeById(*dinner->recipeId);
        REQUIRE(breakfastRecipe);
        REQUIRE(lunchRecipe);
        REQUIRE(dinnerRecipe);

        // The dinner category represents both main-meal positions.
        CHECK(lunchRecipe->mealType == "dinner");
        CHECK(dinnerRecipe->mealType == "dinner");

        if (day < 4) {
            // The seed catalog has four breakfast recipes. Each is used once
            // before the generator starts using the agreed fallback types.
            CHECK(breakfastRecipe->mealType == "breakfast");
            firstBreakfastRecipes.insert(breakfastRecipe->recipeId);
        } else {
            CHECK((breakfastRecipe->mealType == "dinner" || breakfastRecipe->mealType == "dessert"));
        }
    }
    CHECK(firstBreakfastRecipes.size() == 4);
}

TEST_CASE("Catalog generator applies dietary and allergen IDs as hard filters", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Account account;
    account.setAllergenIds({1});
    Preferences preferences;
    preferences.setDietaryTagIds({4});
    REQUIRE(preferences.setBudget(100000.0));

    const auto compatible = catalog.findCompatibleRecipes(
        preferences.getDietaryTagIds(),
        account.getAllergenIds()
    );
    std::unordered_set<int> compatibleIds;
    for (const Recipe& recipe : compatible) {
        compatibleIds.insert(recipe.recipeId);
    }

    MealPlan mealPlan;
    MealGenerator generator;
    REQUIRE(generator.generateWeeklyMealPlan(
        mealPlan,
        catalog,
        account,
        preferences,
        Ingredients{}
    ));

    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        for (const MealType mealType : {MealType::Breakfast, MealType::Lunch, MealType::Dinner}) {
            const MealEntry* meal = mealPlan.getMeal(day, mealType);
            REQUIRE(meal != nullptr);
            REQUIRE(meal->recipeId.has_value());
            CHECK(compatibleIds.count(*meal->recipeId) == 1);
        }
    }
}
