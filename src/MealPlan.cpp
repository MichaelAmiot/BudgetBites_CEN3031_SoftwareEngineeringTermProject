#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/RecipeDataBase.h"

#include <ostream>
#include <utility>


bool MealEntry::isEmpty() const {
    return !recipeId.has_value();

}

MealPlan::MealPlan(std::string planName)
    : planName_(std::move(planName)) {

    if  (planName_.empty()) {
        planName_ = "Weekly Meal Plan";

    }
}

void MealPlan::setPlanName(const std::string& planName) {
    if (!planName.empty()) {
        planName_ = planName;
    }



}

const std::string& MealPlan::getPlanName() const {
    return planName_;

}

bool MealPlan::setMeal(std::size_t dayIndex, MealType mealType, int recipeId) {
    if (recipeId <= 0) {
        return false;
    }

    MealEntry* meal = selectMeal(dayIndex, mealType);
    if (meal == nullptr) {
        return false;
    }

    meal->recipeId = recipeId;
    return true;
}

bool MealPlan::setMeal(
    std::size_t dayIndex,
    MealType mealType,
    const std::string& recipeName
) {
    RecipeDataBase catalog;
    if (!catalog.isLoaded()) {
        return false;
    }

    RecipeFilter filter;
    filter.titleContains = recipeName;
    for (const Recipe& recipe : catalog.searchRecipes(filter)) {
        if (recipe.title == recipeName) {
            return setMeal(dayIndex, mealType, recipe.recipeId);
        }
    }
    return false;
}

bool MealPlan::setMeal(
    std::size_t dayIndex,
    MealType mealType,
    const std::string& recipeName,
    double legacyEstimatedCost
) {
    (void)legacyEstimatedCost;
    return setMeal(dayIndex, mealType, recipeName);
}

const MealEntry* MealPlan::getMeal(std::size_t dayIndex,
    MealType mealType) const {


    return selectMeal(dayIndex, mealType);

}

const DailyMealPlan* MealPlan::getDay(std::size_t dayIndex) const {

    if (dayIndex >= kDaysInWeek) {
        return nullptr;

    }


    return &days_[dayIndex];

}


bool MealPlan::clearMeal(std::size_t dayIndex, MealType mealType) {


    MealEntry* meal = selectMeal(dayIndex, mealType);


    if (meal == nullptr) {
        return false;

    }


    // assigns default mealEntry and clears name and cost

    *meal = MealEntry{};
    return true;

}


void MealPlan::clearAllMeals() {
    days_ = {}; //resets everyday


}

std::size_t MealPlan::countMeals() const {

    std::size_t total=0;

    for (const DailyMealPlan& day : days_) {
        if (!day.breakfast.isEmpty()) {
            ++total;
        }

        if (!day.lunch.isEmpty()) {
            ++total;


        }

        if (!day.dinner.isEmpty()) {
            ++total;
        }
    }
    return total;


}


bool MealPlan::isComplete() const {
    //seven day meal plan 21 meals
    return countMeals() == kDaysInWeek *3;
}


void MealPlan::display(std::ostream& output) const {
    output << "\n=== " << planName_ << " ===\n";

    for (std::size_t dayIndex = 0; dayIndex < kDaysInWeek; ++dayIndex ) {
        const DailyMealPlan& day = days_[dayIndex];

        output<< "\n" << dayName(dayIndex) << ":\n";

        const auto printMeal = [&output](const char* label, const MealEntry& meal) {
            output << " " << label<<": ";

            if (meal.isEmpty()) {
                output << "Not selected\n";
            }else {
                output << "Recipe #" << *meal.recipeId << '\n';
            }
        };

        printMeal("Breakfast", day.breakfast);
        printMeal("Lunch", day.lunch);
        printMeal("Dinner", day.dinner);

    }
}

void MealPlan::display(std::ostream& output, const RecipeDataBase& catalog) const {
    output << "\n=== " << planName_ << " ===\n";

    for (std::size_t dayIndex = 0; dayIndex < kDaysInWeek; ++dayIndex) {
        const DailyMealPlan& day = days_[dayIndex];
        output << "\n" << dayName(dayIndex) << ":\n";

        const auto printMeal = [&output, &catalog](const char* label, const MealEntry& meal) {
            output << " " << label << ": ";
            if (meal.isEmpty()) {
                output << "Not selected\n";
                return;
            }

            const auto recipe = catalog.getRecipeById(*meal.recipeId);
            if (!recipe) {
                output << "Recipe #" << *meal.recipeId << " is unavailable\n";
                return;
            }
            output << recipe->title << " [" << recipe->primaryEquipment << "]\n";
        };

        printMeal("Breakfast", day.breakfast);
        printMeal("Lunch", day.lunch);
        printMeal("Dinner", day.dinner);
    }
}

std::string MealPlan::dayName(std::size_t dayIndex) {
    static const std::array<std::string, kDaysInWeek> kDayNames = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"


    };


    if (dayIndex >= kDaysInWeek) {
        return "Invalid Day";

    }

    return kDayNames[dayIndex];


}
MealEntry* MealPlan::selectMeal(std::size_t dayIndex, MealType mealType) {

    if (dayIndex >= kDaysInWeek) {
        return nullptr;


    }

    DailyMealPlan& day = days_[dayIndex];

    switch (mealType) {

        case MealType::Breakfast:
            return &day.breakfast;
        case MealType::Lunch:
            return &day.lunch;
        case MealType::Dinner:
            return &day.dinner;
    }


    return nullptr;
}


const MealEntry* MealPlan::selectMeal(std::size_t dayIndex, MealType mealType) const {
    if (dayIndex >= kDaysInWeek) {
        return nullptr;
    }


    const DailyMealPlan& day = days_[dayIndex];


    switch (mealType) {
        case MealType::Breakfast:
            return &day.breakfast;
        case MealType::Lunch:
            return &day.lunch;
        case MealType::Dinner:
            return &day.dinner;
    }

    return nullptr;
}
