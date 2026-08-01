#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "BudgetBitesLib/Grocery.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/RecipeDataBase.h"

#include <algorithm>
#include <filesystem>

using Catch::Approx;

TEST_CASE("Grocery adds an item and calculates the total", "[grocery]") {
    Grocery grocery;

    REQUIRE(grocery.addItem(22, 500.0, 2, 6.00));

    REQUIRE(grocery.calculateTotal() == Approx(6.00));
    REQUIRE(grocery.getItems().size() == 1);
    CHECK(grocery.getItems()[0].ingredientId == 22);
}

TEST_CASE("Duplicate grocery items are combined", "[grocery]") {
    Grocery grocery;

    REQUIRE(grocery.addItem(22, 200.0, 1, 4.50));
    REQUIRE(grocery.addItem(22, 100.0, 1, 3.00));

    REQUIRE(grocery.getItems().size() == 1);
    REQUIRE(grocery.getItems()[0].requiredGrams == 300.0);
    REQUIRE(grocery.getItems()[0].purchaseUnits == 2);
    REQUIRE(grocery.calculateTotal() == Approx(7.50));
}

TEST_CASE("Budget comparison works correctly", "[grocery]") {
    Grocery grocery;

    REQUIRE(grocery.addItem(1, 250.0, 1, 4.00));
    REQUIRE(grocery.addItem(2, 250.0, 1, 3.50));

    REQUIRE(grocery.isWithinBudget(10.00));
    REQUIRE_FALSE(grocery.isWithinBudget(5.00));
}

TEST_CASE("Removing an item updates the grocery list", "[grocery]") {
    Grocery grocery;

    REQUIRE(grocery.addItem(1, 250.0, 1, 4.00));
    REQUIRE(grocery.addItem(2, 250.0, 1, 3.50));

    grocery.removeItem(1);

    REQUIRE(grocery.getItems().size() == 1);
    REQUIRE(grocery.getItems()[0].ingredientId == 2);
}

TEST_CASE("Grocery builds a list from recipes and pantry amounts", "[grocery]") {
    const auto seedDirectory = std::filesystem::path(BUDGETBITES_SOURCE_DIR) / "data" / "seed";
    RecipeDataBase catalog(seedDirectory);
    REQUIRE(catalog.isLoaded());

    MealPlan mealPlan;
    REQUIRE(mealPlan.setMeal(0, MealType::Breakfast, 1));

    Ingredients pantry;
    REQUIRE(pantry.addIngredient(152, 300.0));

    Grocery grocery;
    grocery.buildFromMealPlan(mealPlan, catalog, pantry);

    REQUIRE_FALSE(grocery.getItems().empty());
    CHECK(std::none_of(grocery.getItems().begin(), grocery.getItems().end(), [](const GroceryItem& item) {
        return item.ingredientId == 152;
    }));
    CHECK(grocery.calculateTotal() > 0.0);
}

TEST_CASE("Unknown ingredient amounts share purchase units across the week", "[grocery]") {
    const auto seedDirectory = std::filesystem::path(BUDGETBITES_SOURCE_DIR) / "data" / "seed";
    RecipeDataBase catalog(seedDirectory);
    REQUIRE(catalog.isLoaded());

    // Recipe 1 contains olive oil without an exact gram amount. Three uses
    // should estimate two purchase units instead of buying three packages.
    MealPlan mealPlan;
    REQUIRE(mealPlan.setMeal(0, MealType::Breakfast, 1));
    REQUIRE(mealPlan.setMeal(0, MealType::Lunch, 1));
    REQUIRE(mealPlan.setMeal(0, MealType::Dinner, 1));

    Grocery grocery;
    grocery.buildFromMealPlan(mealPlan, catalog, Ingredients{});

    const auto oliveOil = std::find_if(
        grocery.getItems().begin(),
        grocery.getItems().end(),
        [](const GroceryItem& item) { return item.ingredientId == 133; }
    );
    REQUIRE(oliveOil != grocery.getItems().end());
    CHECK(oliveOil->purchaseUnits == 2);
}
