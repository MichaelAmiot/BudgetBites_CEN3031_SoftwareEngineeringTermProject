
#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/Ingredients.h"

#include <iostream>
#include <sstream>
#include <vector>


TEST_CASE("Ingredients list starts empty", "[Ingredients]") {
    Ingredients ingredients;



    CHECK(ingredients.countIngredients() == 0);
}

TEST_CASE("Ingredient can be added", "[Ingredients]") {


    Ingredients ingredients;

    CHECK(ingredients.addIngredient("Chicken"));



    CHECK(ingredients.hasIngredient("Chicken"));
    CHECK(ingredients.countIngredients() == 1);
}

TEST_CASE("Empty ingredient cannot be added", "[Ingredients]") {
    Ingredients ingredients;



    CHECK_FALSE(ingredients.addIngredient(""));
    CHECK(ingredients.countIngredients() == 0);
}

TEST_CASE("Duplicate ingredient cannot be added", "[Ingredients]") {
    Ingredients ingredients;



    REQUIRE(ingredients.addIngredient("Rice"));

    CHECK_FALSE(ingredients.addIngredient("Rice"));



    CHECK(ingredients.countIngredients() == 1);
}

TEST_CASE("All ingredients can be cleared", "[Ingredients]") {
    Ingredients ingredients;



    REQUIRE(ingredients.addIngredient("Eggs"));
    REQUIRE(ingredients.addIngredient("Bread"));



    ingredients.clearIngredients();

    CHECK(ingredients.countIngredients() == 0);
}

TEST_CASE("Pantry items use ingredient IDs and optional grams", "[Ingredients]") {
    Ingredients ingredients;

    REQUIRE(ingredients.addIngredient(22, 300.0));
    REQUIRE(ingredients.addIngredient(153));
    CHECK_FALSE(ingredients.addIngredient(22, 100.0));
    CHECK(ingredients.hasIngredient(22));
    CHECK(ingredients.hasIngredient(153));
    REQUIRE(ingredients.getPantryItems().size() == 2);
    CHECK(ingredients.getPantryItems()[0].availableGrams == 300.0);
    CHECK_FALSE(ingredients.getPantryItems()[1].availableGrams.has_value());
}

TEST_CASE("Pantry item can be removed by ingredient ID", "[Ingredients]") {
    Ingredients ingredients;
    REQUIRE(ingredients.addIngredient(22, 300.0));
    REQUIRE(ingredients.addIngredient(153));

    CHECK(ingredients.removeIngredient(22));
    CHECK_FALSE(ingredients.hasIngredient(22));
    CHECK(ingredients.countIngredients() == 1);
}

TEST_CASE("Ingredients lets the user enter optional pantry grams", "[Ingredients]") {
    Ingredients ingredients;
    const std::vector<Ingredient> available = {
        {22, "Chicken breast", "", 0.0, 500, "package"},
        {153, "Rice", "", 0.0, 1000, "bag"}
    };

    std::istringstream input("22\n300\n153\n\n0\n");
    std::ostringstream output;
    std::streambuf* originalInput = std::cin.rdbuf(input.rdbuf());
    std::streambuf* originalOutput = std::cout.rdbuf(output.rdbuf());
    ingredients.enterIngredients(available);
    std::cin.rdbuf(originalInput);
    std::cout.rdbuf(originalOutput);

    REQUIRE(ingredients.getPantryItems().size() == 2);
    CHECK(ingredients.getPantryItems()[0].ingredientId == 22);
    CHECK(ingredients.getPantryItems()[0].availableGrams == 300.0);
    CHECK(ingredients.getPantryItems()[1].ingredientId == 153);
    CHECK_FALSE(ingredients.getPantryItems()[1].availableGrams.has_value());
}
