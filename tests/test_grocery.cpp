#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "BudgetBitesLib/Grocery.h"

using Catch::Approx;

TEST_CASE("Grocery adds an item and calculates the total", "[grocery]") {
    Grocery grocery;

    grocery.addItem("Rice", 2.0, "bags", 3.00);

    REQUIRE(grocery.calculateTotal() == Approx(6.00));
    REQUIRE(grocery.getItems().size() == 1);
}

TEST_CASE("Duplicate grocery items are combined", "[grocery]") {
    Grocery grocery;

    grocery.addItem("Chicken Breast", 2.0, "lb", 4.50);
    grocery.addItem("Chicken Breast", 1.0, "lb", 4.50);

    REQUIRE(grocery.getItems().size() == 1);
    REQUIRE(grocery.getItems()[0].quantity == Approx(3.0));
    REQUIRE(grocery.calculateTotal() == Approx(13.50));
}

TEST_CASE("Budget comparison works correctly", "[grocery]") {
    Grocery grocery;

    grocery.addItem("Milk", 1.0, "gallon", 4.00);
    grocery.addItem("Eggs", 1.0, "dozen", 3.50);

    REQUIRE(grocery.isWithinBudget(10.00));
    REQUIRE_FALSE(grocery.isWithinBudget(5.00));
}

TEST_CASE("Removing an item updates the grocery list", "[grocery]") {
    Grocery grocery;

    grocery.addItem("Milk", 1.0, "gallon", 4.00);
    grocery.addItem("Eggs", 1.0, "dozen", 3.50);

    grocery.removeItem("Milk");

    REQUIRE(grocery.getItems().size() == 1);
    REQUIRE(grocery.getItems()[0].name == "Eggs");
}