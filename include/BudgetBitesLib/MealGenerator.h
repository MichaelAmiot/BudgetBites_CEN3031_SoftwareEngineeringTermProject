//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//
#pragma once
#ifndef BUDGETBITES_MEALGENERATOR_H
#define BUDGETBITES_MEALGENERATOR_H

#include <vector>
#include <string>

#include "MealPlan.h"
class MealGenerator {

public: void generateWeeklyMealPlan(
    MealPlan& mealPlan,
    const std::vector<std::string>& recipes);





};


#endif //BUDGETBITES_MEALGENERATOR_H