//
// Created by Kezia Saint-Hilaire on 7/29/2026.
#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/MealGenerator.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/RecipeDataBase.h"

#include <filesystem>
#include <unordered_map>
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

TEST_CASE("Catalog generator limits recipe use to three when candidates are available", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Preferences preferences;
    preferences.setDietaryTagIds({2});
    REQUIRE(preferences.setBudget(100.0));

    MealPlan mealPlan;
    MealGenerator generator;
    REQUIRE(generator.generateWeeklyMealPlan(
        mealPlan,
        catalog,
        Account{},
        preferences,
        Ingredients{}
    ));

    std::unordered_map<int, int> recipeUseCounts;
    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        for (const MealType mealType : {MealType::Breakfast, MealType::Lunch, MealType::Dinner}) {
            const MealEntry* meal = mealPlan.getMeal(day, mealType);
            REQUIRE(meal != nullptr);
            REQUIRE(meal->recipeId.has_value());
            ++recipeUseCounts[*meal->recipeId];
        }
    }

    for (const auto& entry : recipeUseCounts) {
        CHECK(entry.second <= 3);
    }
}

TEST_CASE("Catalog generator remains complete under maximum catalog restrictions", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Preferences preferences;
    std::vector<int> dietaryTagIds;
    for (const DietaryTag& tag : catalog.getDietaryTags()) {
        dietaryTagIds.push_back(tag.dietaryTagId);
    }
    preferences.setDietaryTagIds(dietaryTagIds);
    REQUIRE(preferences.setBudget(100.0));

    Account account;
    std::vector<int> allergenIds;
    for (const Allergen& allergen : catalog.getAllergens()) {
        allergenIds.push_back(allergen.allergenId);
    }
    account.setAllergenIds(allergenIds);

    const auto compatible = catalog.findCompatibleRecipes(dietaryTagIds, allergenIds);
    REQUIRE_FALSE(compatible.empty());

    MealPlan mealPlan;
    MealGenerator generator;
    REQUIRE(generator.generateWeeklyMealPlan(
        mealPlan,
        catalog,
        account,
        preferences,
        Ingredients{}
    ));
    CHECK(mealPlan.isComplete());

    std::unordered_map<int, int> recipeUseCounts;
    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        for (const MealType mealType : {MealType::Breakfast, MealType::Lunch, MealType::Dinner}) {
            const MealEntry* meal = mealPlan.getMeal(day, mealType);
            REQUIRE(meal != nullptr);
            REQUIRE(meal->recipeId.has_value());
            ++recipeUseCounts[*meal->recipeId];
        }
    }
    for (const auto& entry : recipeUseCounts) {
        CHECK(entry.second <= 3);
    }
}

TEST_CASE("Budget-first mode uses the available allowance before falling back to the cheapest plan", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Account account;
    account.setAllergenIds({6});
    Preferences lowBudgetPreferences;
    lowBudgetPreferences.setDietaryTagIds({4});
    REQUIRE(lowBudgetPreferences.setBudget(0.0));
    Preferences highBudgetPreferences = lowBudgetPreferences;
    REQUIRE(highBudgetPreferences.setBudget(100.0));

    MealGenerator generator;
    MealPlan lowBudgetPlan;
    MealPlan highBudgetPlan;
    const MealGenerationResult lowBudgetResult = generator.generateWeeklyMealPlan(
        lowBudgetPlan,
        catalog,
        account,
        lowBudgetPreferences,
        Ingredients{},
        MealGenerationMode::BudgetFirst
    );
    const MealGenerationResult highBudgetResult = generator.generateWeeklyMealPlan(
        highBudgetPlan,
        catalog,
        account,
        highBudgetPreferences,
        Ingredients{},
        MealGenerationMode::BudgetFirst
    );

    REQUIRE(lowBudgetResult.complete);
    REQUIRE(highBudgetResult.complete);
    CHECK_FALSE(lowBudgetResult.withinBudget);
    CHECK(highBudgetResult.withinBudget);
    CHECK(highBudgetResult.estimatedCost <= highBudgetPreferences.getBudget() * 1.10);
    CHECK(highBudgetResult.estimatedCost >= lowBudgetResult.estimatedCost);
}

TEST_CASE("Budget-first mode stays within its ten-percent allowance when possible", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Preferences preferences;
    preferences.setDietaryTagIds({2});
    REQUIRE(preferences.setBudget(100.0));

    MealGenerator generator;
    MealPlan normalPlan;
    MealPlan budgetPlan;
    const MealGenerationResult normalResult = generator.generateWeeklyMealPlan(
        normalPlan,
        catalog,
        Account{},
        preferences,
        Ingredients{},
        MealGenerationMode::Normal
    );
    const MealGenerationResult budgetResult = generator.generateWeeklyMealPlan(
        budgetPlan,
        catalog,
        Account{},
        preferences,
        Ingredients{},
        MealGenerationMode::BudgetFirst
    );

    REQUIRE(normalResult.complete);
    REQUIRE(budgetResult.complete);
    CHECK(budgetResult.estimatedCost <= preferences.getBudget() * 1.10);
}

TEST_CASE("Strict-budget mode may return a partial plan without exceeding budget", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Preferences preferences;
    REQUIRE(preferences.setBudget(40.0));

    MealPlan mealPlan;
    MealGenerator generator;
    const MealGenerationResult result = generator.generateWeeklyMealPlan(
        mealPlan,
        catalog,
        Account{},
        preferences,
        Ingredients{},
        MealGenerationMode::StrictBudget
    );

    REQUIRE(result.generated);
    CHECK_FALSE(result.complete);
    CHECK(result.withinBudget);
    CHECK(result.mealsGenerated == mealPlan.countMeals());
    CHECK(result.mealsGenerated < MealPlan::kDaysInWeek * 3);
    CHECK(result.estimatedCost <= preferences.getBudget());
}

TEST_CASE("Strict-budget mode creates a complete plan when the budget is sufficient", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Preferences preferences;
    REQUIRE(preferences.setBudget(100.0));

    MealPlan mealPlan;
    MealGenerator generator;
    const MealGenerationResult result = generator.generateWeeklyMealPlan(
        mealPlan,
        catalog,
        Account{},
        preferences,
        Ingredients{},
        MealGenerationMode::StrictBudget
    );

    CHECK(result.generated);
    CHECK(result.complete);
    CHECK(result.withinBudget);
    CHECK(result.mealsGenerated == MealPlan::kDaysInWeek * 3);
    CHECK(result.estimatedCost <= preferences.getBudget());
}

TEST_CASE("Strict-budget mode returns an empty plan for a zero budget", "[MealGenerator]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Preferences preferences;
    REQUIRE(preferences.setBudget(0.0));

    MealPlan mealPlan;
    MealGenerator generator;
    const MealGenerationResult result = generator.generateWeeklyMealPlan(
        mealPlan,
        catalog,
        Account{},
        preferences,
        Ingredients{},
        MealGenerationMode::StrictBudget
    );

    CHECK_FALSE(result.generated);
    CHECK_FALSE(result.complete);
    CHECK(result.withinBudget);
    CHECK(result.mealsGenerated == 0);
    CHECK(result.estimatedCost == 0.0);
    CHECK(mealPlan.countMeals() == 0);
}
