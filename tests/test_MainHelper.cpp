#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Grocery.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/MainHelper.h"
#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/RecipeDataBase.h"

#include <filesystem>
#include <sstream>

namespace {

std::filesystem::path catalogPath() {
    return std::filesystem::path(BUDGETBITES_SOURCE_DIR) / "data" / "seed";
}

} // namespace

TEST_CASE("MainHelper resets all signed-in session state", "[mainhelper]") {
    Account account;
    account.setAllergenIds({1});
    Preferences preferences;
    REQUIRE(preferences.setBudget(45.0));
    preferences.setDietaryTagIds({4});
    Ingredients ingredients;
    REQUIRE(ingredients.addIngredient(22, 300.0));
    MealPlan mealPlan;
    REQUIRE(mealPlan.setMeal(0, MealType::Dinner, 1));
    Grocery grocery;
    REQUIRE(grocery.addItem(22, 300.0, 1, 4.0));

    MainHelper::resetSessionState(account, preferences, ingredients, mealPlan, grocery);

    CHECK(account.getAllergenIds().empty());
    CHECK(preferences.getDietaryTagIds().empty());
    CHECK(preferences.getBudget() == 0.0);
    CHECK(ingredients.getPantryItems().empty());
    CHECK(mealPlan.countMeals() == 0);
    CHECK(grocery.getItems().empty());
}

TEST_CASE("MainHelper replaces a compatible meal and rebuilds grocery data", "[mainhelper]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());

    Account account;
    Preferences preferences;
    REQUIRE(preferences.setBudget(1000.0));
    Ingredients ingredients;
    MealPlan mealPlan;
    Grocery grocery;

    REQUIRE(MainHelper::replaceMeal(
        mealPlan,
        grocery,
        0,
        MealType::Dinner,
        1,
        catalog,
        account,
        preferences,
        ingredients
    ));
    const MealEntry* dinner = mealPlan.getMeal(0, MealType::Dinner);
    REQUIRE(dinner != nullptr);
    REQUIRE(dinner->recipeId.has_value());
    CHECK(*dinner->recipeId == 1);
    CHECK_FALSE(grocery.getItems().empty());
}

TEST_CASE("MainHelper displays catalog recipe details for a planned meal", "[mainhelper]") {
    RecipeDataBase catalog(catalogPath());
    REQUIRE(catalog.isLoaded());
    MealPlan mealPlan;
    REQUIRE(mealPlan.setMeal(0, MealType::Dinner, 1));

    std::ostringstream output;
    REQUIRE(MainHelper::displayRecipeDetails(
        mealPlan,
        0,
        MealType::Dinner,
        catalog,
        output
    ));
    CHECK(output.str().find("20-Minute Beef Stroganoff") != std::string::npos);
    CHECK(output.str().find("Equipment: oven") != std::string::npos);
    CHECK(output.str().find("Instructions:") != std::string::npos);
}
