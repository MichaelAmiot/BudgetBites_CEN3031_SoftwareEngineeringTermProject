#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>

class RecipeDataBase;

enum class MealType {
    Breakfast,
    Lunch,
    Dinner
};

struct MealEntry {
    std::optional<int> recipeId;

    bool isEmpty() const;
};

struct DailyMealPlan {
    MealEntry breakfast;
    MealEntry lunch;
    MealEntry dinner;
};

class MealPlan {
public:
    static constexpr std::size_t kDaysInWeek = 7;

    explicit MealPlan(std::string planName = "Weekly Meal Plan");
    void setPlanName(const std::string& planName);
    const std::string& getPlanName() const;

    bool setMeal(
        std::size_t dayIndex,
        MealType mealType,
        int recipeId
    );

    // Kept for code that still passes recipe titles.
    bool setMeal(
        std::size_t dayIndex,
        MealType mealType,
        const std::string& recipeName
    );

    // Kept for code that still passes the old estimated cost.
    bool setMeal(
        std::size_t dayIndex,
        MealType mealType,
        const std::string& recipeName,
        double legacyEstimatedCost
    );

    const MealEntry* getMeal(std::size_t dayIndex, MealType mealType) const;
    const DailyMealPlan* getDay(std::size_t dayIndex) const;
    bool clearMeal(std::size_t dayIndex, MealType mealType);
    void clearAllMeals();
    std::size_t countMeals() const;
    bool isComplete() const;

    void display(std::ostream& output) const;
    void display(std::ostream& output, const RecipeDataBase& catalog) const;

    static std::string dayName(std::size_t dayIndex);

private:
    std::string planName_;
    std::array<DailyMealPlan, kDaysInWeek> days_{};

    MealEntry* selectMeal(std::size_t dayIndex, MealType mealType);
    const MealEntry* selectMeal(std::size_t dayIndex, MealType mealType) const;
};
