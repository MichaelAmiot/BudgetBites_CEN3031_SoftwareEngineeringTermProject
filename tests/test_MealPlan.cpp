#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>

#include "BudgetBitesLib/MealPlan.h"

TEST_CASE("New mean plan starts on empty", "[MealPlan]") {


    MealPlan plan("My First Plan");

    CHECK(plan.getPlanName() == "My First Plan");



    CHECK(plan.countMeals() == 0);
    CHECK_FALSE(plan.isComplete());


}

TEST_CASE("Meals are added", "[MealPlan]") {
    MealPlan plan;

    REQUIRE(plan.setMeal(0, MealType::Breakfast, 1));


    REQUIRE(plan.setMeal(0, MealType::Lunch, 2));


    const MealEntry* breakfast = plan.getMeal(0, MealType::Breakfast);


    const MealEntry* lunch = plan.getMeal(0, MealType::Lunch);

    REQUIRE(breakfast != nullptr);


    REQUIRE(lunch != nullptr);

    CHECK(breakfast->recipeId == 1);
    CHECK(lunch->recipeId == 2);
    CHECK(plan.countMeals() == 2);
}

TEST_CASE("A plan is complete after all 21 meal positions are filled", "[MealPlan]") {
    MealPlan plan;

    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        REQUIRE(plan.setMeal(day, MealType::Breakfast, 1));
        REQUIRE(plan.setMeal(day, MealType::Lunch, 2));
        REQUIRE(plan.setMeal(day, MealType::Dinner, 3));
    }

    CHECK(plan.countMeals() == 21);
    CHECK(plan.isComplete());
}

TEST_CASE("A meal can be cleared after storing a recipe ID", "[MealPlan]") {
    MealPlan plan;
    REQUIRE(plan.setMeal(0, MealType::Dinner, 3));

    REQUIRE(plan.clearMeal(0, MealType::Dinner));
    const MealEntry* dinner = plan.getMeal(0, MealType::Dinner);
    REQUIRE(dinner != nullptr);
    CHECK(dinner->isEmpty());
    CHECK(plan.countMeals() == 0);
}
