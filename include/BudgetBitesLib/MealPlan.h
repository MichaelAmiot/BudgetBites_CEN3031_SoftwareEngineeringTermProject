#pragma once   // prevents double-inclusion


#ifndef BUDGETBITESLIB_MEALPLAN_H
#define BUDGETBITESLIB_MEALPLAN_H

#include <array>
#include <cstddef>
#include <iosfwd>
#include <string>

//using this to represent the three meal positions

enum class MealType {
    Breakfast,
    Lunch,
    Dinner
};

//stores the info needed to display in a weekely plan

struct MealEntry {
    std:: string recipeName;
    double estimatedCost = 0.0;

    bool isEmpty() const;
};


//stores all three meals in one day
struct DailyMealPlan {
    MealEntry breakfast;
    MealEntry lunch;
    MealEntry dinner;

};



//stores 7 day meal plan
//Day index : 0 = monday. 1= tuesday, 6= sunday
class MealPlan {
public:
    static constexpr std::size_t kDaysInWeek = 7;


    //creates an empty plan
    explicit MealPlan (std::string planName = "Weekly Meal Plan");

    // does the changing and returning of the plans name
    void setPlanName(const std::string& planName);


    const std::string& getPlanName() const;


    //this adds or relaces a meal
    //also returns false when dayIndex is outside 0-6
    // recipeName is empty, or when estimatedCost is negative
    bool setMeal(std::size_t dayIndex,
        MealType mealType,
        const std::string& recipeName,
        double estimatedCost =0.0);


    const MealEntry* getMeal(std::size_t dayIndex, MealType mealType) const; //returns a pointer to meal oe nullptr

    const DailyMealPlan* getDay(std::size_t dayIndex) const; // returns point to full day plan or npt when dayIndex isnt valid


    // removes only one meal
    bool clearMeal(std::size_t dayIndex, MealType mealType);
    //removes all the meals
    void clearAllMeals();
    std::size_t countMeals() const; //Counts how many of the 21 meal positions contains a recipe currently


    bool isComplete() const; //true only when all 21 positions are full
    double getTotalEstimatedCost() const; // add estimated costs of all meals in the plan

    //prints weekly plan
    void display(std::ostream& output) const;

    static std:: string dayName(std::size_t dayIndex); //converts day index to name like "monday"

private:

    std:: string planName_;
    std::array<DailyMealPlan, kDaysInWeek> days_{};


    MealEntry* selectMeal(std::size_t dayIndex, MealType mealType);
    const MealEntry* selectMeal(std::size_t dayIndex, MealType mealType) const;


};




 #endif