#include "BudgetBitesLib/MainHelper.h"

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Grocery.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/Preferences.h"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>

namespace {

std::string compactRecipeLabel(const Recipe& recipe) {
    constexpr std::size_t maximumLength = 38;
    const std::string label = std::to_string(recipe.recipeId) + ": " + recipe.title;
    return label.size() <= maximumLength
        ? label
        : label.substr(0, maximumLength - 3) + "...";
}

} // namespace

void MainHelper::resetSessionState(
    Account& account,
    Preferences& preferences,
    Ingredients& ingredients,
    MealPlan& mealPlan,
    Grocery& grocery
) {
    account = Account{};
    preferences = Preferences{};
    ingredients = Ingredients{};
    mealPlan = MealPlan{};
    grocery = Grocery{};
}

void MainHelper::displayBudgetStatus(
    const Grocery& grocery,
    const Preferences& preferences,
    std::ostream& output
) {
    const double total = grocery.calculateTotal();
    const double budget = preferences.getBudget();
    output << std::fixed << std::setprecision(2)
           << "Weekly budget: $" << budget << '\n'
           << "Estimated grocery total: $" << total << '\n';

    if (grocery.isWithinBudget(budget)) {
        output << "Within budget by $" << budget - total << "\n";
    } else {
        output << "Warning: estimated cost is over budget by $" << total - budget << "\n";
    }
}

bool MainHelper::displayRecipeDetails(
    const MealPlan& mealPlan,
    std::size_t dayIndex,
    MealType mealType,
    const RecipeDataBase& catalog,
    std::ostream& output
) {
    const MealEntry* meal = mealPlan.getMeal(dayIndex, mealType);
    if (!meal || !meal->recipeId) {
        return false;
    }

    const auto recipe = catalog.getRecipeById(*meal->recipeId);
    if (!recipe) {
        return false;
    }

    output << "\n=== " << recipe->title << " ===\n"
           << "Meal type: " << recipe->mealType << '\n'
           << "Difficulty: " << recipe->difficulty << '\n'
           << "Equipment: " << recipe->primaryEquipment << '\n';
    if (recipe->prepMinutes) {
        output << "Preparation time: " << *recipe->prepMinutes << " minutes\n";
    }
    if (recipe->cookMinutes) {
        output << "Cooking time: " << *recipe->cookMinutes << " minutes\n";
    }

    output << "\nIngredients:\n";
    for (const RecipeIngredient& ingredient : catalog.getRecipeIngredients(recipe->recipeId)) {
        output << "- " << ingredient.sourceIngredientText << '\n';
    }

    const auto seasoners = catalog.getRecipeSeasoners(recipe->recipeId);
    if (!seasoners.empty()) {
        output << "\nSeasoners:\n";
        for (const std::string& seasoner : seasoners) {
            output << "- " << seasoner << '\n';
        }
    }

    const auto instructions = catalog.getPreparationInstructions(recipe->recipeId);
    output << "\nInstructions:\n"
           << (instructions ? *instructions : "No instructions available.") << '\n';
    return true;
}

std::vector<Recipe> MainHelper::compatibleRecipesForMeal(
    MealType mealType,
    const RecipeDataBase& catalog,
    const Account& account,
    const Preferences& preferences
) {
    const auto compatible = catalog.findCompatibleRecipes(
        preferences.getDietaryTagIds(),
        account.getAllergenIds()
    );

    std::vector<Recipe> result;
    for (const Recipe& recipe : compatible) {
        const bool allowedBreakfast =
            mealType == MealType::Breakfast &&
            (recipe.mealType == "breakfast" || recipe.mealType == "dinner" || recipe.mealType == "dessert");
        const bool allowedMainMeal =
            mealType != MealType::Breakfast && recipe.mealType == "dinner";
        if (allowedBreakfast || allowedMainMeal) {
            result.push_back(recipe);
        }
    }
    return result;
}

void MainHelper::displayRecipeOptions(
    const std::vector<Recipe>& recipes,
    std::ostream& output
) {
    constexpr int columnWidth = 44;
    constexpr std::size_t columns = 2;
    for (std::size_t index = 0; index < recipes.size(); ++index) {
        output << std::left << std::setw(columnWidth) << compactRecipeLabel(recipes[index]);
        if ((index + 1) % columns == 0 || index + 1 == recipes.size()) {
            output << '\n';
        }
    }
    output << std::right;
}

bool MainHelper::replaceMeal(
    MealPlan& mealPlan,
    Grocery& grocery,
    std::size_t dayIndex,
    MealType mealType,
    int recipeId,
    const RecipeDataBase& catalog,
    const Account& account,
    const Preferences& preferences,
    const Ingredients& ingredients
) {
    const auto candidates = compatibleRecipesForMeal(mealType, catalog, account, preferences);
    const bool validRecipe = std::any_of(candidates.begin(), candidates.end(), [&](const Recipe& recipe) {
        return recipe.recipeId == recipeId;
    });
    if (!validRecipe || !mealPlan.setMeal(dayIndex, mealType, recipeId)) {
        return false;
    }

    grocery.buildFromMealPlan(mealPlan, catalog, ingredients);
    return true;
}
