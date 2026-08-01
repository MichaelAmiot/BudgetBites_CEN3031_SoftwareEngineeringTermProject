//
// Created by Kezia Saint-Hilaire on 7/29/2026.
#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/Preferences.h"

#include <iostream>
#include <sstream>
#include <vector>


TEST_CASE("Preferences can be saved", "[Preferences]") {


    Preferences preferences;

    CHECK(preferences.addPreference("Vegetarian"));

    CHECK(preferences.getPreferences().size() == 1);




    CHECK(preferences.getPreferences()[0] == "Vegetarian");
}


TEST_CASE("Multiple preferences can be saved", "[Preferences]") {
    Preferences preferences;

    CHECK(preferences.addPreference("Vegetarian"));


    CHECK(preferences.addPreference("Gluten-Free"));

    CHECK(preferences.getPreferences().size() == 2);


    CHECK(preferences.getPreferences()[0] == "Vegetarian");

    CHECK(preferences.getPreferences()[1] == "Gluten-Free");
}


TEST_CASE("Duplicate preferences are rejected", "[Preferences]") {
    Preferences preferences;



    REQUIRE(preferences.addPreference("Vegetarian"));


    CHECK_FALSE(preferences.addPreference("Vegetarian"));
    CHECK(preferences.getPreferences().size() == 1);
}

TEST_CASE("Budget can be saved", "[Preferences]") {



    Preferences preferences;

    REQUIRE(preferences.setBudget(50.0));


    CHECK(preferences.getBudget() == 50.0);
}

TEST_CASE("Negative budget is rejected", "[Preferences]") {
    Preferences preferences;

    REQUIRE(preferences.setBudget(50.0));
    CHECK_FALSE(preferences.setBudget(-1.0));
    CHECK(preferences.getBudget() == 50.0);
}

TEST_CASE("Dietary tag IDs can be saved without duplicates", "[Preferences]") {
    Preferences preferences;

    CHECK(preferences.addDietaryTagId(1));
    CHECK(preferences.addDietaryTagId(4));
    CHECK_FALSE(preferences.addDietaryTagId(1));
    CHECK(preferences.getDietaryTagIds() == std::vector<int>{1, 4});
}

TEST_CASE("Preferences lets the user select from available dietary tags", "[Preferences]") {
    Preferences preferences;
    const std::vector<DietaryTag> available = {
        {1, "vegan", "Vegan", "diet_pattern", "No animal-derived ingredients."},
        {2, "vegetarian", "Vegetarian", "diet_pattern", "No meat or seafood."},
        {4, "dairy_free", "Dairy-free", "exclusion", "No milk-derived ingredients."}
    };

    std::istringstream input("1\n4\n0\n");
    std::ostringstream output;
    std::streambuf* originalInput = std::cin.rdbuf(input.rdbuf());
    std::streambuf* originalOutput = std::cout.rdbuf(output.rdbuf());
    preferences.enterDietaryPreferences(available);
    std::cin.rdbuf(originalInput);
    std::cout.rdbuf(originalOutput);

    CHECK(preferences.getDietaryTagIds() == std::vector<int>{1, 4});
    CHECK(output.str().find("Vegan") != std::string::npos);
    CHECK(output.str().find("Dairy-free") != std::string::npos);
}
