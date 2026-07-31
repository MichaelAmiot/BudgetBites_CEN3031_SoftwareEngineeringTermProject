
#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/Ingredients.h"



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