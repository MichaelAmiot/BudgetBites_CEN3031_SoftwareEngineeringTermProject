#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>

#include "BudgetBitesLib/MealPlan.h"

TEST_CASE("New mean plan starts on empty", "[MealPlan]") {


    MealPlan plan("My First Plan");

    CHECK(plan.getPlanName() == "My First Plan");



    CHECK(plan.countMeals() == 0);
    CHECK_FALSE(plan.isComplete());


    CHECK(plan.getTotalEstimatedCost() == 0.0);
}

TEST_CASE("Meals are added", "[MealPlan]") {
    MealPlan plan;

    REQUIRE(plan.setMeal(0, MealType::Breakfast, "Oatmeal", 1.25));


    REQUIRE(plan.setMeal(0, MealType::Lunch, "Chicken Wrap", 3.75));


    const MealEntry* breakfast = plan.getMeal(0, MealType::Breakfast);


    const MealEntry* lunch = plan.getMeal(0, MealType::Lunch);

    REQUIRE(breakfast != nullptr);


    REQUIRE(lunch != nullptr);

    CHECK(breakfast->recipeName == "Oatmeal");

    CHECK(breakfast->estimatedCost == 1.25);


    CHECK(lunch->recipeName == "Chicken Wrap");
    CHECK(plan.countMeals() == 2);

    CHECK(plan.getTotalEstimatedCost() == 5.00);
}

TEST_CASE("A plan is complete after all 21 meal positions are filled", "[MealPlan]") {
    MealPlan plan;

    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        REQUIRE(plan.setMeal(day, MealType::Breakfast, "Breakfast", 1.00));
        REQUIRE(plan.setMeal(day, MealType::Lunch, "Lunch", 2.00));
        REQUIRE(plan.setMeal(day, MealType::Dinner, "Dinner", 3.00));
    }

    CHECK(plan.countMeals() == 21);
    CHECK(plan.isComplete());
    CHECK(plan.getTotalEstimatedCost() == 42.00);
}
