//
// Created by Kezia Saint-Hilaire on 7/29/2026.
#include <catch2/catch_test_macros.hpp>

#include "BudgetBitesLib/Preferences.h"



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

    preferences.setBudget(50.0);


    CHECK(preferences.getBudget() == 50.0);
}