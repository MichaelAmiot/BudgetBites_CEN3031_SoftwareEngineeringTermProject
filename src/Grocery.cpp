#include "BudgetBitesLib/Grocery.h"

#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/RecipeDataBase.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <unordered_map>

namespace {

struct IngredientRequirement {
    double knownGrams = 0.0;
    int unknownAmountCount = 0;
};

} // namespace

bool Grocery::addItem(
    int ingredientId,
    std::optional<double> requiredGrams,
    int purchaseUnits,
    double estimatedCost
) {
    if (ingredientId <= 0 ||
        (requiredGrams && *requiredGrams < 0.0) ||
        purchaseUnits <= 0 ||
        estimatedCost < 0.0) {
        return false;
    }

    for (GroceryItem& item : items_) {
        if (item.ingredientId != ingredientId) {
            continue;
        }
        if (item.requiredGrams && requiredGrams) {
            *item.requiredGrams += *requiredGrams;
        } else {
            item.requiredGrams = std::nullopt;
        }
        item.purchaseUnits += purchaseUnits;
        item.estimatedCost += estimatedCost;
        return true;
    }

    items_.push_back({ingredientId, requiredGrams, purchaseUnits, estimatedCost});
    return true;
}

void Grocery::removeItem(int ingredientId) {
    items_.erase(
        std::remove_if(items_.begin(), items_.end(), [&](const GroceryItem& item) {
            return item.ingredientId == ingredientId;
        }),
        items_.end()
    );
}

void Grocery::buildFromMealPlan(
    const MealPlan& mealPlan,
    const RecipeDataBase& catalog,
    const Ingredients& pantry
) {
    items_.clear();

    std::vector<int> recipeIds;
    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        for (const MealType mealType : {MealType::Breakfast, MealType::Lunch, MealType::Dinner}) {
            const MealEntry* meal = mealPlan.getMeal(day, mealType);
            if (meal && meal->recipeId) {
                recipeIds.push_back(*meal->recipeId);
            }
        }
    }

    // Combine the full week's per-person requirements before rounding to packages.
    std::unordered_map<int, IngredientRequirement> requirementsByIngredient;
    for (const RecipeCostItem& costItem : catalog.getRecipeCostItems(recipeIds)) {
        IngredientRequirement& requirement = requirementsByIngredient[costItem.ingredientId];
        if (costItem.requiredGrams) {
            const auto recipe = catalog.getRecipeById(costItem.recipeId);
            const int servings = recipe && recipe->servings && *recipe->servings > 0
                ? *recipe->servings
                : 1;
            requirement.knownGrams += *costItem.requiredGrams / servings;
        } else {
            ++requirement.unknownAmountCount;
        }
    }

    for (const auto& entry : requirementsByIngredient) {
        const int ingredientId = entry.first;
        const IngredientRequirement& requirement = entry.second;
        const auto ingredient = catalog.getIngredientById(ingredientId);
        if (!ingredient) {
            continue;
        }

        // Unknown quantities share packages: 1-2 occurrences use one unit,
        // 3-5 use two units, 6-8 use three units, and so on.
        double estimatedRequiredGrams = requirement.knownGrams;
        if (requirement.unknownAmountCount > 0) {
            const int estimatedUnknownUnits = 1 + requirement.unknownAmountCount / 3;
            estimatedRequiredGrams = std::max(
                estimatedRequiredGrams,
                estimatedUnknownUnits * static_cast<double>(ingredient->purchaseUnitGrams)
            );
        }

        const auto pantryItem = std::find_if(pantry.getPantryItems().begin(), pantry.getPantryItems().end(), [&](const PantryItem& item) {
            return item.ingredientId == ingredientId;
        });
        if (pantryItem != pantry.getPantryItems().end()) {
            // An unmeasured pantry item is treated as available to avoid a duplicate purchase.
            if (!pantryItem->availableGrams) {
                continue;
            }
            estimatedRequiredGrams -= *pantryItem->availableGrams;
            if (estimatedRequiredGrams <= 0.0) {
                continue;
            }
        }

        const int purchaseUnits = std::max(
            1,
            static_cast<int>(std::ceil(estimatedRequiredGrams / ingredient->purchaseUnitGrams))
        );
        const double unitCost = ingredient->pricePer100Grams * ingredient->purchaseUnitGrams / 100.0;
        addItem(ingredientId, estimatedRequiredGrams, purchaseUnits, purchaseUnits * unitCost);
    }
}

double Grocery::calculateTotal() const {
    double total = 0.0;
    for (const GroceryItem& item : items_) {
        total += item.estimatedCost;
    }
    return total;
}

bool Grocery::isWithinBudget(double weeklyBudget) const {
    return weeklyBudget >= 0.0 && calculateTotal() <= weeklyBudget;
}

void Grocery::displayList() const {
    std::cout << "\n=== Grocery List ===\n";
    if (items_.empty()) {
        std::cout << "The grocery list is empty.\n";
        return;
    }
    for (const GroceryItem& item : items_) {
        std::cout << "Ingredient #" << item.ingredientId << ": "
                  << item.purchaseUnits << " unit(s) - $"
                  << std::fixed << std::setprecision(2) << item.estimatedCost << '\n';
    }
    std::cout << "Estimated total: $" << std::fixed << std::setprecision(2) << calculateTotal() << '\n';
}

void Grocery::displayList(const RecipeDataBase& catalog) const {
    std::cout << "\n=== Grocery List ===\n";
    if (items_.empty()) {
        std::cout << "The grocery list is empty.\n";
        return;
    }

    for (const GroceryItem& item : items_) {
        const auto ingredient = catalog.getIngredientById(item.ingredientId);
        const std::string name = ingredient ? ingredient->name : "Ingredient #" + std::to_string(item.ingredientId);
        const std::string unitLabel = ingredient ? ingredient->purchaseUnitLabel : "unit";
        std::cout << name << ": " << item.purchaseUnits << " x " << unitLabel;
        if (item.requiredGrams) {
            std::cout << " (" << *item.requiredGrams << " g needed)";
        }
        std::cout << " - $" << std::fixed << std::setprecision(2) << item.estimatedCost << '\n';
    }
    std::cout << "Estimated total: $" << std::fixed << std::setprecision(2) << calculateTotal() << '\n';
}

const std::vector<GroceryItem>& Grocery::getItems() const {
    return items_;
}
