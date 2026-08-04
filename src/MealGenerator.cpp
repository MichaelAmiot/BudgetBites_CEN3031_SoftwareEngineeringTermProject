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
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

using RecipeCandidates = std::vector<const Recipe*>;
constexpr int kPreferredMaximumRecipeUses = 3;

// One shared random engine for the generator. Seeded once from a real
//  entropy source (not time-of-day or a fixed seed) so back-to-back plans
//  in the same run, and plans across different runs, don't repeat the
//  same "random" order.
// NOSONAR-justification: this PRNG only selects which recipe fills a meal
//  slot. It never generates anything security-sensitive (tokens, passwords,
//  session IDs, salts), so a predictable/non-cryptographic generator is
//  intentional and safe here. See PasswordSecurity.cpp for the engine used
//  for actual security-sensitive randomness in this project.
std::mt19937& sharedRandomEngine() { // NOSONAR
    static thread_local std::mt19937 engine(std::random_device{}()); // NOSONAR
    return engine;
}

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
    int previousUses;

    // Explicit constructor so emplace_back can build this in place directly
    //  from its arguments under C++17. Without this, CandidateEvaluation is
    //  a plain aggregate, and emplace_back(args...) on an aggregate needs
    //  C++20's parenthesized aggregate initialization to compile.
    CandidateEvaluation(
        const Recipe* recipe_,
        double score_,
        double totalCost_,
        bool withinBudget_,
        int previousUses_
    ) : recipe(recipe_),
        score(score_),
        totalCost(totalCost_),
        withinBudget(withinBudget_),
        previousUses(previousUses_) {}
};

bool isBetterNormalCandidate(
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

bool isBetterBudgetCandidate(
    const CandidateEvaluation& candidate,
    const CandidateEvaluation& currentBest
) {
    // Grocery already includes pantry amounts, package sizes, and ingredient reuse.
    if (candidate.totalCost != currentBest.totalCost) {
        return candidate.totalCost < currentBest.totalCost;
    }
    if (candidate.previousUses != currentBest.previousUses) {
        return candidate.previousUses < currentBest.previousUses;
    }
    return candidate.recipe->recipeId < currentBest.recipe->recipeId;
}

std::optional<CandidateEvaluation> chooseCandidate(
    RecipeCandidates candidates,
    std::size_t day,
    MealType mealType,
    MealGenerationMode mode,
    std::unordered_map<int, int>& recipeUseCount,
    const std::unordered_set<int>& pantryIngredientIds,
    const std::unordered_set<int>& plannedIngredientIds,
    const MealPlan& generatedPlan,
    const RecipeDataBase& catalog,
    const Ingredients& pantry,
    double weeklyBudget,
    double currentCost,
    int maximumRecipeUses
) {
    if (mode == MealGenerationMode::Normal) {
        RecipeCandidates candidatesBelowUseLimit;
        for (const Recipe* candidate : candidates) {
            if (recipeUseCount[candidate->recipeId] < kPreferredMaximumRecipeUses) {
                candidatesBelowUseLimit.push_back(candidate);
            }
        }
        if (!candidatesBelowUseLimit.empty()) {
            candidates = std::move(candidatesBelowUseLimit);
        }
    } else if (maximumRecipeUses > 0) {
        // Budget modes try several plans with progressively looser repeat limits.
        candidates.erase(
            std::remove_if(
                candidates.begin(),
                candidates.end(),
                [&](const Recipe* candidate) {
                    return recipeUseCount[candidate->recipeId] >= maximumRecipeUses;
                }
            ),
            candidates.end()
        );
    }

    std::vector<CandidateEvaluation> evaluations;
    for (const Recipe* candidate : candidates) {
        const std::unordered_set<int> ingredientIds = recipeIngredientIds(catalog, candidate->recipeId);
        int pantryMatches = 0;
        int plannedReuseMatches = 0;
        for (const int ingredientId : ingredientIds) {
            if (pantryIngredientIds.count(ingredientId) > 0) {
                ++pantryMatches;
            }
            if (plannedIngredientIds.count(ingredientId) > 0) {
                ++plannedReuseMatches;
            }
        }

        // Build a trial grocery list so package sizes and reused ingredients are included.
        MealPlan trialPlan = generatedPlan;
        trialPlan.setMeal(day, mealType, candidate->recipeId);
        Grocery trialGrocery;
        trialGrocery.buildFromMealPlan(trialPlan, catalog, pantry);
        const double trialCost = trialGrocery.calculateTotal();
        if (mode == MealGenerationMode::StrictBudget && trialCost > weeklyBudget) {
            continue;
        }

        const double additionalCost = std::max(0.0, trialCost - currentCost);
        const double score =
            pantryMatches * 3.0 +
            plannedReuseMatches * 2.0 -
            recipeUseCount[candidate->recipeId] * 2.0 -
            additionalCost;

        evaluations.emplace_back(
            candidate,
            score,
            trialCost,
            trialCost <= weeklyBudget,
            recipeUseCount[candidate->recipeId]
        );
    }

    if (evaluations.empty()) {
        return std::nullopt;
    }

    using ComparatorFn = bool (*)(const CandidateEvaluation&, const CandidateEvaluation&);
    const ComparatorFn isBetter = (mode == MealGenerationMode::Normal)
        ? static_cast<ComparatorFn>(isBetterNormalCandidate)
        : static_cast<ComparatorFn>(isBetterBudgetCandidate);
    // NOSONAR-justification: SonarCloud suggests std::ranges::sort, which
    //  needs C++20. This project targets C++17 (see CMAKE_CXX_STANDARD in
    //  CMakeLists.txt), so the classic iterator-pair std::sort is the
    //  correct, compilable form here.
    std::sort(evaluations.begin(), evaluations.end(), isBetter); // NOSONAR

    // Pick randomly among the best few options instead of always the single
    //  best, but only in Normal mode. Every candidate here already passed the
    //  allergy/dietary hard filter upstream (see findCompatibleRecipes above),
    //  so this only adds variety among choices that are already safe and
    //  close to optimal, it never picks something worse than "one of the
    //  best few" for this slot. This is why pressing Generate Weekly Plan
    //  again in Normal mode produces a different plan instead of the exact
    //  same one every time.
    //  Budget-First and Strict-Budget stay fully deterministic (always the
    //  single cheapest/best-scoring option) because their whole purpose is
    //  squeezing cost as tightly as possible; picking a runner-up there
    //  could push the plan over budget for no reason.
    constexpr std::size_t kVarietyPoolSize = 3;
    const std::size_t poolSize = (mode == MealGenerationMode::Normal)
        ? std::min(kVarietyPoolSize, evaluations.size())
        : std::size_t{1};
    std::uniform_int_distribution<std::size_t> pick(0, poolSize - 1);
    return evaluations[pick(sharedRandomEngine())];
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
    const MealGenerationResult result = generateWeeklyMealPlan(
        mealPlan,
        catalog,
        account,
        preferences,
        pantry,
        MealGenerationMode::Normal
    );
    return result.generated && result.complete;
}

MealGenerationResult MealGenerator::generateWeeklyMealPlan(
    MealPlan& mealPlan,
    const RecipeDataBase& catalog,
    const Account& account,
    const Preferences& preferences,
    const Ingredients& pantry,
    MealGenerationMode mode
) {
    MealGenerationResult result;
    if (!catalog.isLoaded() || preferences.getBudget() < 0.0) {
        return result;
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
        return result;
    }

    std::unordered_set<int> pantryIngredientIds;
    for (const PantryItem& item : pantry.getPantryItems()) {
        pantryIngredientIds.insert(item.ingredientId);
    }

    const double weeklyBudget = preferences.getBudget();

    struct GenerationAttempt {
        MealPlan plan;
        MealGenerationResult result;
        int maximumRecipeUses;
    };

    // Reuse the existing greedy generator for each repeat limit.
    const auto generateAttempt = [&](int maximumRecipeUses) {
        GenerationAttempt attempt{
            MealPlan(mealPlan.getPlanName()),
            MealGenerationResult{},
            maximumRecipeUses
        };
        MealPlan& generatedPlan = attempt.plan;
        std::unordered_map<int, int> recipeUseCount;
        std::unordered_set<int> plannedIngredientIds;
        double currentCost = 0.0;

        // Normal modes fill each day in display order. Strict mode first gives every
        // day a dinner, then adds lunches and breakfasts while money remains.
        std::vector<std::pair<std::size_t, MealType>> mealSlots;
        if (mode == MealGenerationMode::StrictBudget) {
            for (const MealType mealType : {MealType::Dinner, MealType::Lunch, MealType::Breakfast}) {
                for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
                    mealSlots.emplace_back(day, mealType);
                }
            }
        } else {
            for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
                for (const MealType mealType : {MealType::Breakfast, MealType::Lunch, MealType::Dinner}) {
                    mealSlots.emplace_back(day, mealType);
                }
            }
        }

        for (const auto& slot : mealSlots) {
            const std::size_t day = slot.first;
            const MealType mealType = slot.second;
            RecipeCandidates candidates;
            RecipeCandidates strictBreakfastFallback;

            if (mealType == MealType::Breakfast) {
                // Use each compatible breakfast once before the agreed dinner or
                // dessert fallback. Strict mode may use the fallback sooner if the
                // remaining budget cannot afford any unused breakfast.
                for (const Recipe* recipe : breakfastRecipes) {
                    if (recipeUseCount[recipe->recipeId] == 0) {
                        candidates.push_back(recipe);
                    }
                }
                if (candidates.empty()) {
                    candidates.insert(candidates.end(), dinnerRecipes.begin(), dinnerRecipes.end());
                    candidates.insert(candidates.end(), dessertRecipes.begin(), dessertRecipes.end());
                } else if (mode == MealGenerationMode::StrictBudget) {
                    strictBreakfastFallback.insert(
                        strictBreakfastFallback.end(),
                        dinnerRecipes.begin(),
                        dinnerRecipes.end()
                    );
                    strictBreakfastFallback.insert(
                        strictBreakfastFallback.end(),
                        dessertRecipes.begin(),
                        dessertRecipes.end()
                    );
                }
                if (candidates.empty()) {
                    candidates = breakfastRecipes;
                }
            } else {
                // The dinner catalog category represents both main-meal positions.
                candidates = dinnerRecipes;
            }

            std::optional<CandidateEvaluation> best = chooseCandidate(
                candidates,
                day,
                mealType,
                mode,
                recipeUseCount,
                pantryIngredientIds,
                plannedIngredientIds,
                generatedPlan,
                catalog,
                pantry,
                weeklyBudget,
                currentCost,
                maximumRecipeUses
            );
            if (!best && mode == MealGenerationMode::StrictBudget && !strictBreakfastFallback.empty()) {
                best = chooseCandidate(
                    strictBreakfastFallback,
                    day,
                    mealType,
                    mode,
                    recipeUseCount,
                    pantryIngredientIds,
                    plannedIngredientIds,
                    generatedPlan,
                    catalog,
                    pantry,
                    weeklyBudget,
                    currentCost,
                    maximumRecipeUses
                );
            }
            if (!best) {
                if (mode == MealGenerationMode::StrictBudget) {
                    continue;
                }
                break;
            }

            if (!generatedPlan.setMeal(day, mealType, best->recipe->recipeId)) {
                break;
            }
            ++recipeUseCount[best->recipe->recipeId];
            const auto selectedIngredients = recipeIngredientIds(catalog, best->recipe->recipeId);
            plannedIngredientIds.insert(selectedIngredients.begin(), selectedIngredients.end());
            currentCost = best->totalCost;
        }

        Grocery finalGrocery;
        finalGrocery.buildFromMealPlan(generatedPlan, catalog, pantry);
        attempt.result.mealsGenerated = generatedPlan.countMeals();
        attempt.result.generated = attempt.result.mealsGenerated > 0;
        attempt.result.complete = generatedPlan.isComplete();
        attempt.result.estimatedCost = finalGrocery.calculateTotal();
        attempt.result.withinBudget = attempt.result.estimatedCost <= weeklyBudget;
        return attempt;
    };

    if (mode == MealGenerationMode::Normal) {
        GenerationAttempt attempt = generateAttempt(0);
        mealPlan = attempt.plan;
        return attempt.result;
    }

    std::vector<GenerationAttempt> attempts;
    for (const int maximumRecipeUses : {3, 5, 0}) {
        attempts.push_back(generateAttempt(maximumRecipeUses));
    }

    const auto hasStricterRepeatLimit = [](const GenerationAttempt& left, const GenerationAttempt& right) {
        return left.maximumRecipeUses > 0 &&
            (right.maximumRecipeUses == 0 || left.maximumRecipeUses < right.maximumRecipeUses);
    };

    const GenerationAttempt* selected = nullptr;
    if (mode == MealGenerationMode::BudgetFirst) {
        const double allowedCost = weeklyBudget * 1.10;
        // Prefer the complete candidate that uses as much of the 110% allowance as possible.
        for (const GenerationAttempt& attempt : attempts) {
            if (!attempt.result.complete || attempt.result.estimatedCost > allowedCost) {
                continue;
            }
            if (!selected ||
                attempt.result.estimatedCost > selected->result.estimatedCost ||
                (attempt.result.estimatedCost == selected->result.estimatedCost &&
                 hasStricterRepeatLimit(attempt, *selected))) {
                selected = &attempt;
            }
        }
        // If 21 meals cannot fit in the allowance, retain the cheapest complete plan.
        if (!selected) {
            for (const GenerationAttempt& attempt : attempts) {
                if (!attempt.result.complete) {
                    continue;
                }
                if (!selected ||
                    attempt.result.estimatedCost < selected->result.estimatedCost ||
                    (attempt.result.estimatedCost == selected->result.estimatedCost &&
                     hasStricterRepeatLimit(attempt, *selected))) {
                    selected = &attempt;
                }
            }
        }
    } else {
        // Strict mode first preserves meal count, then spends the available budget.
        for (const GenerationAttempt& attempt : attempts) {
            if (!selected ||
                attempt.result.mealsGenerated > selected->result.mealsGenerated ||
                (attempt.result.mealsGenerated == selected->result.mealsGenerated &&
                 attempt.result.estimatedCost > selected->result.estimatedCost) ||
                (attempt.result.mealsGenerated == selected->result.mealsGenerated &&
                 attempt.result.estimatedCost == selected->result.estimatedCost &&
                 hasStricterRepeatLimit(attempt, *selected))) {
                selected = &attempt;
            }
        }
    }

    if (!selected) {
        return result;
    }
    mealPlan = selected->plan;
    return selected->result;
}
