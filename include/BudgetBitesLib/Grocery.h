#pragma once   // prevents double-inclusion

#include <optional>
#include <vector>

class Ingredients;
class MealPlan;
class RecipeDataBase;

struct GroceryItem {
    int ingredientId;
    std::optional<double> requiredGrams;
    int purchaseUnits = 0;
    double estimatedCost = 0.0;
};

class Grocery {
public:
    // Adds or merges the requirements for one catalog ingredient.
    bool addItem(int ingredientId,
                 std::optional<double> requiredGrams,
                 int purchaseUnits,
                 double estimatedCost);

    void removeItem(int ingredientId);

    // Rebuilds the shopping list from the selected recipes and pantry amounts.
    void buildFromMealPlan(const MealPlan& mealPlan,
                           const RecipeDataBase& catalog,
                           const Ingredients& pantry);

    double calculateTotal() const;

    bool isWithinBudget(double weeklyBudget) const;

    // Displays IDs when no catalog is available.
    void displayList() const;

    // Displays ingredient names and purchase-unit labels from the catalog.
    void displayList(const RecipeDataBase& catalog) const;

    const std::vector<GroceryItem>& getItems() const;

private:
    std::vector<GroceryItem> items_;
};
