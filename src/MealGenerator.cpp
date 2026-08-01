//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//

#include "BudgetBitesLib/MealGenerator.h"

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Grocery.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/RecipeDataBase.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace {

using RecipeCandidates = std::vector<const Recipe*>;

std::unordered_set<int> recipeIngredientIds(
    const RecipeDataBase& catalog,
    int recipeId
) {
    std::unordered_set<int> ingredientIds;
    for (const RecipeIngredient& ingredient : catalog.getRecipeIngredients(recipeId)) {
        ingredientIds.insert(ingredient.ingredientId);
    }
    return ingredientIds;
}

struct CandidateEvaluation {
    const Recipe* recipe;
    double score;
    double totalCost;
    bool withinBudget;
};

bool isBetterCandidate(
    const CandidateEvaluation& candidate,
    const CandidateEvaluation& currentBest
) {
    // Any budget-compatible option is preferred over an over-budget option.
    if (candidate.withinBudget != currentBest.withinBudget) {
        return candidate.withinBudget;
    }
    if (candidate.withinBudget) {
        if (candidate.score != currentBest.score) {
            return candidate.score > currentBest.score;
        }
        if (candidate.totalCost != currentBest.totalCost) {
            return candidate.totalCost < currentBest.totalCost;
        }
    } else {
        // When the budget cannot be met, finish the plan with the cheapest option.
        if (candidate.totalCost != currentBest.totalCost) {
            return candidate.totalCost < currentBest.totalCost;
        }
        if (candidate.score != currentBest.score) {
            return candidate.score > currentBest.score;
        }
    }
    return candidate.recipe->recipeId < currentBest.recipe->recipeId;
}

} // namespace

void MealGenerator::generateWeeklyMealPlan(MealPlan& mealPlan, const std::vector<std::string> &recipes) {

    if (recipes.empty()) {

        return;
    }

    std::size_t recipeIndex = 0;

    for (int day=0; day < 7; day++) {

        mealPlan.setMeal(
            day,
            MealType::Breakfast,
            recipes[recipeIndex],
        5.00);
        recipeIndex++;

        if (recipeIndex >= recipes.size()) {
            recipeIndex = 0;


        }

    mealPlan.setMeal(
        day,
        MealType::Lunch,
        recipes[recipeIndex],
        7.50);


        recipeIndex++;

        if (recipeIndex >= recipes.size()) {
            recipeIndex = 0;


        }

        mealPlan.setMeal(
            day,
            MealType::Dinner,
            recipes[recipeIndex],
            10.00);


        recipeIndex++;


        if (recipeIndex >= recipes.size()) {
            recipeIndex=0;
        }


    }

}

void MealGenerator::generateWeeklyMealPlan(
    MealPlan& mealPlan,
    const std::vector<int>& recipeIds
) {
    if (recipeIds.empty()) {
        return;
    }

    std::size_t recipeIndex = 0;
    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        mealPlan.setMeal(day, MealType::Breakfast, recipeIds[recipeIndex]);
        recipeIndex = (recipeIndex + 1) % recipeIds.size();

        mealPlan.setMeal(day, MealType::Lunch, recipeIds[recipeIndex]);
        recipeIndex = (recipeIndex + 1) % recipeIds.size();

        mealPlan.setMeal(day, MealType::Dinner, recipeIds[recipeIndex]);
        recipeIndex = (recipeIndex + 1) % recipeIds.size();
    }
}

bool MealGenerator::generateWeeklyMealPlan(
    MealPlan& mealPlan,
    const RecipeDataBase& catalog,
    const Account& account,
    const Preferences& preferences,
    const Ingredients& pantry
) {
    if (!catalog.isLoaded()) {
        return false;
    }

    // Allergy and dietary conflicts are hard filters and never enter scoring.
    const std::vector<Recipe> compatibleRecipes = catalog.findCompatibleRecipes(
        preferences.getDietaryTagIds(),
        account.getAllergenIds()
    );

    // Divide compatible recipes into the three catalog categories used by planning.
    RecipeCandidates breakfastRecipes;
    RecipeCandidates dinnerRecipes;
    RecipeCandidates dessertRecipes;
    for (const Recipe& recipe : compatibleRecipes) {
        if (recipe.mealType == "breakfast") {
            breakfastRecipes.push_back(&recipe);
        } else if (recipe.mealType == "dinner") {
            dinnerRecipes.push_back(&recipe);
        } else if (recipe.mealType == "dessert") {
            dessertRecipes.push_back(&recipe);
        }
    }
    // Lunch and dinner cannot be filled without at least one main-meal recipe.
    if (dinnerRecipes.empty()) {
        return false;
    }

    std::unordered_set<int> pantryIngredientIds;
    for (const PantryItem& item : pantry.getPantryItems()) {
        pantryIngredientIds.insert(item.ingredientId);
    }

    MealPlan generatedPlan(mealPlan.getPlanName());
    std::unordered_map<int, int> recipeUseCount;
    std::unordered_set<int> plannedIngredientIds;
    double currentCost = 0.0;
    const double weeklyBudget = preferences.getBudget();

    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        for (const MealType mealType : {MealType::Breakfast, MealType::Lunch, MealType::Dinner}) {
            RecipeCandidates candidates;
            if (mealType == MealType::Breakfast) {
                // Use every compatible breakfast once before dinner/dessert fallback.
                for (const Recipe* recipe : breakfastRecipes) {
                    if (recipeUseCount[recipe->recipeId] == 0) {
                        candidates.push_back(recipe);
                    }
                }
                if (candidates.empty()) {
                    candidates.insert(candidates.end(), dinnerRecipes.begin(), dinnerRecipes.end());
                    candidates.insert(candidates.end(), dessertRecipes.begin(), dessertRecipes.end());
                }
                if (candidates.empty()) {
                    candidates = breakfastRecipes;
                }
            } else {
                // The dinner catalog category represents both lunch and dinner.
                candidates = dinnerRecipes;
            }

            if (candidates.empty()) {
                return false;
            }

            std::optional<CandidateEvaluation> best;
            for (const Recipe* candidate : candidates) {
                const std::unordered_set<int> ingredientIds = recipeIngredientIds(catalog, candidate->recipeId);
                int pantryMatches = 0;
                int plannedReuseMatches = 0;
                for (const int ingredientId : ingredientIds) {
                    pantryMatches += pantryIngredientIds.count(ingredientId) ? 1 : 0;
                    plannedReuseMatches += plannedIngredientIds.count(ingredientId) ? 1 : 0;
                }

                // Rebuild a small trial grocery list to obtain the true marginal weekly cost.
                MealPlan trialPlan = generatedPlan;
                trialPlan.setMeal(day, mealType, candidate->recipeId);
                Grocery trialGrocery;
                trialGrocery.buildFromMealPlan(trialPlan, catalog, pantry);
                const double trialCost = trialGrocery.calculateTotal();
                const double additionalCost = std::max(0.0, trialCost - currentCost);

                // Prefer pantry ingredients and ingredient reuse, while reducing
                // unnecessary repetition and additional grocery cost.
                const double score =
                    pantryMatches * 3.0 +
                    plannedReuseMatches * 2.0 -
                    recipeUseCount[candidate->recipeId] * 2.0 -
                    additionalCost;

                const CandidateEvaluation evaluation{
                    candidate,
                    score,
                    trialCost,
                    trialCost <= weeklyBudget
                };
                if (!best || isBetterCandidate(evaluation, *best)) {
                    best = evaluation;
                }
            }

            if (!best || !generatedPlan.setMeal(day, mealType, best->recipe->recipeId)) {
                return false;
            }
            ++recipeUseCount[best->recipe->recipeId];
            const auto selectedIngredients = recipeIngredientIds(catalog, best->recipe->recipeId);
            plannedIngredientIds.insert(selectedIngredients.begin(), selectedIngredients.end());
            currentCost = best->totalCost;
        }
    }

    mealPlan = generatedPlan;
    return mealPlan.isComplete();
}
