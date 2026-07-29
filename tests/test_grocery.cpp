#include <catch2/catch_test_macros.hpp>
#include "BudgetBitesLib/Grocery.h"
TEST_CASE("Placeholder test for Grocery", "[grocery]") {
    REQUIRE(1 == 1);
}
TEST_CASE("Grocery list starts empty", "[Grocery]") {

    Grocery grocery;

    CHECK(grocery.countIngredients() == 0);
}

TEST_CASE("Ingredient can be added", "[Grocery]") {



    Grocery grocery;

    CHECK(grocery.addIngredient("Chicken"));


    CHECK(grocery.hasIngredient("Chicken"));


    CHECK(grocery.countIngredients() == 1);
}

TEST_CASE("Empty ingredient cannot be added", "[Grocery]") {



    Grocery grocery;

    CHECK_FALSE(grocery.addIngredient(""));

    CHECK(grocery.countIngredients() == 0);
}



TEST_CASE("Duplicate ingredient cannot be added", "[Grocery]") {
    Grocery grocery;



    REQUIRE(grocery.addIngredient("Rice"));

    CHECK_FALSE(grocery.addIngredient("Rice"));




    CHECK(grocery.countIngredients() == 1);
}

TEST_CASE("All ingredients can be cleared", "[Grocery]") {



    Grocery grocery;

    REQUIRE(grocery.addIngredient("Eggs"));




    REQUIRE(grocery.addIngredient("Bread"));

    grocery.clearIngredients();




    CHECK(grocery.countIngredients() == 0);
}