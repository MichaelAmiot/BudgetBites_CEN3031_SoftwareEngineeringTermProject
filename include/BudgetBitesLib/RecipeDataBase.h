#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Recipe {
    int recipeId;
    std::string title;
    std::optional<int> servings;
    std::optional<int> prepMinutes;
    std::optional<int> cookMinutes;
    std::string difficulty;
    std::string mealType;
    std::string primaryEquipment;
    std::string selectionNotes;
    std::string sourceName;
    std::string sourceUrl;
};

struct RecipeFilter {
    std::optional<std::string> titleContains;
    std::vector<std::string> mealTypes;
    std::optional<std::string> difficulty;
    std::optional<int> maximumPrepMinutes;
    std::optional<int> maximumCookMinutes;
};

struct Ingredient {
    int ingredientId;
    std::string name;
    std::string description;
    double pricePer100Grams;
    int purchaseUnitGrams;
    std::string purchaseUnitLabel;
};

struct RecipeIngredient {
    int recipeId;
    int ingredientId;
    std::string ingredientName;
    double quantity;
    std::string unit;
    std::optional<double> weightGrams;
    std::string sourceIngredientText;
};

struct RecipeCostItem {
    int recipeId;
    int ingredientId;
    std::string ingredientName;
    std::optional<double> requiredGrams;
    double pricePer100Grams;
    int purchaseUnitGrams;
    std::string purchaseUnitLabel;
};

struct DietaryTag {
    int dietaryTagId;
    std::string code;
    std::string displayName;
    std::string tagGroup;
    std::string description;
};

struct Allergen {
    int allergenId;
    std::string code;
    std::string displayName;
};

struct Seasoner {
    int seasonerId;
    std::string name;
    std::string sourceExamples;
};

// Loads the recipe data and provides read-only queries.
class RecipeDataBase {
public:
    explicit RecipeDataBase(const std::filesystem::path& seedDirectory = defaultSeedDirectory());

    bool load(const std::filesystem::path& seedDirectory);
    bool isLoaded() const noexcept;
    const std::string& lastError() const noexcept;
    static std::filesystem::path defaultSeedDirectory();

    std::optional<Recipe> getRecipeById(int recipeId) const;
    std::vector<Recipe> searchRecipes(const RecipeFilter& filter = {}) const;

    std::optional<Ingredient> getIngredientById(int ingredientId) const;
    std::vector<Ingredient> getAllIngredients() const;
    std::vector<Ingredient> searchIngredients(const std::string& keyword) const;

    std::vector<RecipeIngredient> getRecipeIngredients(int recipeId) const;
    std::vector<RecipeIngredient> getRecipeIngredientsForRecipes(const std::vector<int>& recipeIds) const;
    std::vector<RecipeCostItem> getRecipeCostItems(const std::vector<int>& recipeIds) const;
    std::optional<std::string> getPreparationInstructions(int recipeId) const;
    std::vector<std::string> getRecipeSeasoners(int recipeId) const;

    std::vector<DietaryTag> getDietaryTags() const;
    std::vector<Allergen> getAllergens() const;
    // Filters recipes by dietary tags and allergens.
    std::vector<Recipe> findCompatibleRecipes(
        const std::vector<int>& dietaryTagIds,
        const std::vector<int>& allergenIds
    ) const;

private:
    bool fail(const std::string& message);
    void clear();

    std::unordered_map<int, Recipe> recipesById_;
    std::unordered_map<int, Ingredient> ingredientsById_;
    std::unordered_map<int, std::vector<RecipeIngredient>> recipeIngredientsByRecipe_;
    std::unordered_map<int, std::string> instructionsByRecipe_;
    std::unordered_map<int, Seasoner> seasonersById_;
    std::unordered_map<int, std::vector<int>> seasonersByRecipe_;
    std::unordered_map<int, DietaryTag> dietaryTagsById_;
    std::unordered_map<int, Allergen> allergensById_;
    std::unordered_map<int, std::unordered_set<int>> dietaryConflictsByIngredient_;
    std::unordered_map<int, std::unordered_set<int>> allergensByIngredient_;
    std::unordered_map<int, std::unordered_set<int>> dietaryConflictsBySeasoner_;
    std::unordered_map<int, std::unordered_set<int>> allergensBySeasoner_;
    bool loaded_ = false;
    std::string lastError_;
};
