#include "BudgetBitesLib/RecipeDataBase.h"

#include "BudgetBitesLib/CsvFile.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <sstream>

namespace {

using Row = std::vector<std::string>;

bool hasHeaders(const CsvFile::Table& table, const std::vector<std::string>& expected) {
    return table.headers == expected;
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::optional<int> integer(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }
    try {
        std::size_t used = 0;
        const int result = std::stoi(value, &used);
        return used == value.size() ? std::optional<int>(result) : std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> real(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }
    try {
        std::size_t used = 0;
        const double result = std::stod(value, &used);
        return used == value.size() ? std::optional<double>(result) : std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<std::string> splitSeasoners(const std::string& seasoners) {
    std::vector<std::string> values;
    std::istringstream input(seasoners);
    std::string value;
    while (std::getline(input, value, ',')) {
        const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char character) {
            return std::isspace(character) != 0;
        });
        const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char character) {
            return std::isspace(character) != 0;
        }).base();
        if (first < last) {
            values.emplace_back(first, last);
        }
    }
    return values;
}

template <typename Value>
std::vector<Value> sortedValues(const std::unordered_map<int, Value>& values, const std::function<std::string(const Value&)>& key) {
    std::vector<Value> result;
    result.reserve(values.size());
    for (const auto& entry : values) {
        result.push_back(entry.second);
    }
    std::sort(result.begin(), result.end(), [&](const Value& left, const Value& right) {
        return key(left) < key(right);
    });
    return result;
}

} // namespace

RecipeDataBase::RecipeDataBase(const std::filesystem::path& seedDirectory) {
    load(seedDirectory);
}

bool RecipeDataBase::load(const std::filesystem::path& seedDirectory) {
    clear();
    const auto readTable = [&](const std::string& filename, const std::vector<std::string>& headers, CsvFile::Table& table) {
        std::string error;
        if (!CsvFile::read(seedDirectory / filename, table, error)) {
            return fail(error);
        }
        // The fixed column order keeps each CSV record safe to map by index.
        if (!hasHeaders(table, headers)) {
            return fail(filename + " headers do not match the catalog schema.");
        }
        return true;
    };

    CsvFile::Table recipes;
    if (!readTable("recipes.csv", {"id", "title", "servings", "prep_minutes", "cook_minutes", "difficulty", "meal_type", "primary_equipment", "selection_notes", "source_name", "source_url"}, recipes)) {
        return false;
    }
    for (const Row& row : recipes.rows) {
        const auto id = integer(row[0]);
        const auto servings = integer(row[2]);
        const auto prepMinutes = integer(row[3]);
        const auto cookMinutes = integer(row[4]);
        if (!id || (row[2].size() && !servings) || (row[3].size() && !prepMinutes) || (row[4].size() && !cookMinutes)) {
            return fail("recipes.csv contains an invalid numeric value.");
        }
        if (!recipesById_.emplace(*id, Recipe{*id, row[1], servings, prepMinutes, cookMinutes, row[5], row[6], row[7], row[8], row[9], row[10]}).second) {
            return fail("recipes.csv contains a duplicate recipe ID.");
        }
    }

    CsvFile::Table ingredients;
    if (!readTable("ingredients.csv", {"id", "name", "description", "price_100gm", "purchase_unit_gram", "purchase_unit_label"}, ingredients)) {
        return false;
    }
    for (const Row& row : ingredients.rows) {
        const auto id = integer(row[0]);
        const auto price = real(row[3]);
        const auto grams = integer(row[4]);
        if (!id || !price || !grams || *grams <= 0) {
            return fail("ingredients.csv contains an invalid numeric value.");
        }
        if (!ingredientsById_.emplace(*id, Ingredient{*id, row[1], row[2], *price, *grams, row[5]}).second) {
            return fail("ingredients.csv contains a duplicate ingredient ID.");
        }
    }

    CsvFile::Table recipeIngredients;
    if (!readTable("recipe_ingredients.csv", {"recipe_id", "recipe_name", "ingredient_id", "ingredient_name", "quantity", "unit", "weight_gram", "source_ingredient_text"}, recipeIngredients)) {
        return false;
    }
    for (const Row& row : recipeIngredients.rows) {
        const auto recipeId = integer(row[0]);
        const auto ingredientId = integer(row[2]);
        const auto quantity = real(row[4]);
        const auto weight = real(row[6]);
        if (!recipeId || !ingredientId || !quantity || (row[6].size() && !weight) || !recipesById_.count(*recipeId) || !ingredientsById_.count(*ingredientId)) {
            return fail("recipe_ingredients.csv contains an invalid or unlinked record.");
        }
        recipeIngredientsByRecipe_[*recipeId].push_back({*recipeId, *ingredientId, row[3], *quantity, row[5], weight, row[7]});
    }

    CsvFile::Table instructions;
    if (!readTable("recipe_instructions.csv", {"recipe_id", "preparation_instructions"}, instructions)) {
        return false;
    }
    for (const Row& row : instructions.rows) {
        const auto recipeId = integer(row[0]);
        if (!recipeId || !recipesById_.count(*recipeId) || !instructionsByRecipe_.emplace(*recipeId, row[1]).second) {
            return fail("recipe_instructions.csv contains an invalid or duplicate record.");
        }
    }

    // Recipe seasoners are stored as one comma-separated value per recipe.
    CsvFile::Table seasoners;
    if (!readTable("seasoner.csv", {"id", "name", "source_examples"}, seasoners)) {
        return false;
    }
    CsvFile::Table recipeSeasoners;
    if (!readTable("recipes_seasoner.csv", {"recipe_id", "recipe_name", "seasoners"}, recipeSeasoners)) {
        return false;
    }
    for (const Row& row : recipeSeasoners.rows) {
        const auto recipeId = integer(row[0]);
        if (!recipeId || !recipesById_.count(*recipeId) || !seasonersByRecipe_.emplace(*recipeId, splitSeasoners(row[2])).second) {
            return fail("recipes_seasoner.csv contains an invalid or duplicate record.");
        }
    }

    CsvFile::Table dietaryTags;
    if (!readTable("dietary_tags.csv", {"id", "code", "display_name", "tag_group", "description"}, dietaryTags)) {
        return false;
    }
    for (const Row& row : dietaryTags.rows) {
        const auto id = integer(row[0]);
        if (!id || !dietaryTagsById_.emplace(*id, DietaryTag{*id, row[1], row[2], row[3], row[4]}).second) {
            return fail("dietary_tags.csv contains an invalid or duplicate record.");
        }
    }

    CsvFile::Table allergens;
    if (!readTable("allergens.csv", {"id", "code", "display_name"}, allergens)) {
        return false;
    }
    for (const Row& row : allergens.rows) {
        const auto id = integer(row[0]);
        if (!id || !allergensById_.emplace(*id, Allergen{*id, row[1], row[2]}).second) {
            return fail("allergens.csv contains an invalid or duplicate record.");
        }
    }

    CsvFile::Table ingredientAllergens;
    if (!readTable("ingredient_allergens.csv", {"ingredient_id", "ingredient_name", "allergen_id", "allergen_name", "status", "note"}, ingredientAllergens)) {
        return false;
    }
    for (const Row& row : ingredientAllergens.rows) {
        const auto ingredientId = integer(row[0]);
        const auto allergenId = integer(row[2]);
        if (!ingredientId || !allergenId || !ingredientsById_.count(*ingredientId) || !allergensById_.count(*allergenId)) {
            return fail("ingredient_allergens.csv contains an invalid linked record.");
        }
        // A known relation must be removed for a user with that allergy.
        if (row[4] != "unknown") {
            allergensByIngredient_[*ingredientId].insert(allergensById_.at(*allergenId).code);
        }
    }

    CsvFile::Table ingredientDietaryTags;
    if (!readTable("ingredient_dietary_tags.csv", {"ingredient_id", "ingredient_name", "dietary_tag_id", "dietary_tag_name", "status", "note"}, ingredientDietaryTags)) {
        return false;
    }
    for (const Row& row : ingredientDietaryTags.rows) {
        const auto ingredientId = integer(row[0]);
        const auto tagId = integer(row[2]);
        if (!ingredientId || !tagId || !ingredientsById_.count(*ingredientId) || !dietaryTagsById_.count(*tagId)) {
            return fail("ingredient_dietary_tags.csv contains an invalid linked record.");
        }
        // Treat conditional rows as conflicts for a safer default result.
        if (row[4] == "incompatible" || row[4] == "conditional") {
            dietaryConflictsByIngredient_[*ingredientId].insert(dietaryTagsById_.at(*tagId).code);
        }
    }

    loaded_ = true;
    return true;
}

bool RecipeDataBase::isLoaded() const noexcept {
    return loaded_;
}

const std::string& RecipeDataBase::lastError() const noexcept {
    return lastError_;
}

std::filesystem::path RecipeDataBase::defaultSeedDirectory() {
    return "data/seed";
}

std::optional<Recipe> RecipeDataBase::getRecipeById(int recipeId) const {
    const auto found = recipesById_.find(recipeId);
    return found == recipesById_.end() ? std::nullopt : std::optional<Recipe>(found->second);
}

std::vector<Recipe> RecipeDataBase::searchRecipes(const RecipeFilter& filter) const {
    std::vector<Recipe> result;
    const auto titleNeedle = filter.titleContains ? lower(*filter.titleContains) : "";
    for (const auto& entry : recipesById_) {
        const Recipe& recipe = entry.second;
        const bool mealMatches = filter.mealTypes.empty() || std::find(filter.mealTypes.begin(), filter.mealTypes.end(), recipe.mealType) != filter.mealTypes.end();
        if ((!titleNeedle.empty() && lower(recipe.title).find(titleNeedle) == std::string::npos) ||
            !mealMatches ||
            (filter.difficulty && recipe.difficulty != *filter.difficulty) ||
            (filter.maximumPrepMinutes && (!recipe.prepMinutes || *recipe.prepMinutes > *filter.maximumPrepMinutes)) ||
            (filter.maximumCookMinutes && (!recipe.cookMinutes || *recipe.cookMinutes > *filter.maximumCookMinutes))) {
            continue;
        }
        result.push_back(recipe);
    }
    std::sort(result.begin(), result.end(), [](const Recipe& left, const Recipe& right) {
        return left.title < right.title;
    });
    return result;
}

std::optional<Ingredient> RecipeDataBase::getIngredientById(int ingredientId) const {
    const auto found = ingredientsById_.find(ingredientId);
    return found == ingredientsById_.end() ? std::nullopt : std::optional<Ingredient>(found->second);
}

std::vector<Ingredient> RecipeDataBase::getAllIngredients() const {
    return sortedValues<Ingredient>(ingredientsById_, [](const Ingredient& ingredient) { return ingredient.name; });
}

std::vector<Ingredient> RecipeDataBase::searchIngredients(const std::string& keyword) const {
    const auto needle = lower(keyword);
    std::vector<Ingredient> result;
    for (const auto& entry : ingredientsById_) {
        const Ingredient& ingredient = entry.second;
        if (lower(ingredient.name).find(needle) != std::string::npos) {
            result.push_back(ingredient);
        }
    }
    std::sort(result.begin(), result.end(), [](const Ingredient& left, const Ingredient& right) {
        return left.name < right.name;
    });
    return result;
}

std::vector<RecipeIngredient> RecipeDataBase::getRecipeIngredients(int recipeId) const {
    const auto found = recipeIngredientsByRecipe_.find(recipeId);
    return found == recipeIngredientsByRecipe_.end() ? std::vector<RecipeIngredient>{} : found->second;
}

std::vector<RecipeIngredient> RecipeDataBase::getRecipeIngredientsForRecipes(const std::vector<int>& recipeIds) const {
    std::vector<RecipeIngredient> result;
    for (const int recipeId : recipeIds) {
        const auto ingredients = getRecipeIngredients(recipeId);
        result.insert(result.end(), ingredients.begin(), ingredients.end());
    }
    return result;
}

std::vector<RecipeCostItem> RecipeDataBase::getRecipeCostItems(const std::vector<int>& recipeIds) const {
    std::vector<RecipeCostItem> result;
    for (const RecipeIngredient& recipeIngredient : getRecipeIngredientsForRecipes(recipeIds)) {
        const Ingredient& ingredient = ingredientsById_.at(recipeIngredient.ingredientId);
        result.push_back({recipeIngredient.recipeId, ingredient.id, ingredient.name, recipeIngredient.weightGrams, ingredient.pricePer100Grams, ingredient.purchaseUnitGrams, ingredient.purchaseUnitLabel});
    }
    return result;
}

std::optional<std::string> RecipeDataBase::getPreparationInstructions(int recipeId) const {
    const auto found = instructionsByRecipe_.find(recipeId);
    return found == instructionsByRecipe_.end() ? std::nullopt : std::optional<std::string>(found->second);
}

std::vector<std::string> RecipeDataBase::getRecipeSeasoners(int recipeId) const {
    const auto found = seasonersByRecipe_.find(recipeId);
    return found == seasonersByRecipe_.end() ? std::vector<std::string>{} : found->second;
}

std::vector<DietaryTag> RecipeDataBase::getDietaryTags() const {
    return sortedValues<DietaryTag>(dietaryTagsById_, [](const DietaryTag& tag) { return tag.id < 10 ? "0" + std::to_string(tag.id) : std::to_string(tag.id); });
}

std::vector<Allergen> RecipeDataBase::getAllergens() const {
    return sortedValues<Allergen>(allergensById_, [](const Allergen& allergen) { return allergen.displayName; });
}

std::vector<Recipe> RecipeDataBase::findCompatibleRecipes(
    const std::vector<std::string>& dietaryTagCodes,
    const std::vector<std::string>& allergenCodes
) const {
    std::unordered_set<std::string> requestedDietaryTags(dietaryTagCodes.begin(), dietaryTagCodes.end());
    std::unordered_set<std::string> requestedAllergens(allergenCodes.begin(), allergenCodes.end());
    std::vector<Recipe> result;
    for (const auto& recipeEntry : recipesById_) {
        bool compatible = true;
        for (const RecipeIngredient& recipeIngredient : getRecipeIngredients(recipeEntry.first)) {
            const auto dietary = dietaryConflictsByIngredient_.find(recipeIngredient.ingredientId);
            const auto allergens = allergensByIngredient_.find(recipeIngredient.ingredientId);
            if ((dietary != dietaryConflictsByIngredient_.end() && std::any_of(requestedDietaryTags.begin(), requestedDietaryTags.end(), [&](const std::string& code) { return dietary->second.count(code); })) ||
                (allergens != allergensByIngredient_.end() && std::any_of(requestedAllergens.begin(), requestedAllergens.end(), [&](const std::string& code) { return allergens->second.count(code); }))) {
                compatible = false;
                break;
            }
        }
        if (compatible) {
            result.push_back(recipeEntry.second);
        }
    }
    std::sort(result.begin(), result.end(), [](const Recipe& left, const Recipe& right) {
        return left.title < right.title;
    });
    return result;
}

bool RecipeDataBase::fail(const std::string& message) {
    clear();
    lastError_ = message;
    return false;
}

void RecipeDataBase::clear() {
    recipesById_.clear();
    ingredientsById_.clear();
    recipeIngredientsByRecipe_.clear();
    instructionsByRecipe_.clear();
    seasonersByRecipe_.clear();
    dietaryTagsById_.clear();
    allergensById_.clear();
    dietaryConflictsByIngredient_.clear();
    allergensByIngredient_.clear();
    loaded_ = false;
    lastError_.clear();
}
