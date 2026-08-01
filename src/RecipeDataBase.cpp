#include "BudgetBitesLib/RecipeDataBase.h"

#include "BudgetBitesLib/CsvFile.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

using Row = std::vector<std::string>;

bool loadTable(
    const std::filesystem::path& seedDirectory,
    const std::string& filename,
    const std::vector<std::string>& headers,
    CsvFile::Table& table,
    std::string& error
) {
    if (!CsvFile::read(seedDirectory / filename, table, error)) {
        return false;
    }
    if (table.headers != headers) {
        error = filename + " headers do not match the catalog schema.";
        return false;
    }
    return true;
}

std::string lower(std::string value) {
    for (char& character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

std::optional<int> integer(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }
    try {
        std::size_t used = 0;
        const int result = std::stoi(value, &used);
        if (used != value.size()) {
            return std::nullopt;
        }
        return result;
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
        if (used != value.size()) {
            return std::nullopt;
        }
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<std::string> splitSeasoners(const std::string& seasoners) {
    std::vector<std::string> values;
    std::istringstream input(seasoners);
    std::string value;

    while (std::getline(input, value, ',')) {
        // Remove spaces around each seasoner name.
        std::size_t first = 0;
        while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
            first++;
        }

        std::size_t last = value.size();
        while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
            last--;
        }

        if (first < last) {
            values.push_back(value.substr(first, last - first));
        }
    }
    return values;
}

bool containsAny(const std::unordered_set<int>& values, const std::unordered_set<int>& selectedValues) {
    for (int value : selectedValues) {
        if (values.count(value)) {
            return true;
        }
    }
    return false;
}

} // namespace

RecipeDataBase::RecipeDataBase(const std::filesystem::path& seedDirectory) {
    load(seedDirectory);
}

bool RecipeDataBase::load(const std::filesystem::path& seedDirectory) {
    clear();
    std::string error;

    CsvFile::Table recipes;
    if (!loadTable(
            seedDirectory,
            "recipes.csv",
            {"id", "title", "servings", "prep_minutes", "cook_minutes", "difficulty", "meal_type", "primary_equipment", "selection_notes", "source_name", "source_url"},
            recipes,
            error)) {
        return fail(error);
    }
    for (const Row& row : recipes.rows) {
        const auto id = integer(row[0]);
        const auto servings = integer(row[2]);
        const auto prepMinutes = integer(row[3]);
        const auto cookMinutes = integer(row[4]);
        if (!id || (!row[2].empty() && !servings) || (!row[3].empty() && !prepMinutes) || (!row[4].empty() && !cookMinutes)) {
            return fail("recipes.csv contains an invalid numeric value.");
        }

        Recipe recipe{
            *id,
            row[1],
            servings,
            prepMinutes,
            cookMinutes,
            row[5],
            row[6],
            row[7],
            row[8],
            row[9],
            row[10]
        };
        if (!recipesById_.emplace(*id, recipe).second) {
            return fail("recipes.csv contains a duplicate recipe ID.");
        }
    }

    CsvFile::Table ingredients;
    if (!loadTable(
            seedDirectory,
            "ingredients.csv",
            {"id", "name", "description", "price_100gm", "purchase_unit_gram", "purchase_unit_label"},
            ingredients,
            error)) {
        return fail(error);
    }
    for (const Row& row : ingredients.rows) {
        const auto id = integer(row[0]);
        const auto price = real(row[3]);
        const auto grams = integer(row[4]);
        if (!id || !price || !grams || *grams <= 0) {
            return fail("ingredients.csv contains an invalid numeric value.");
        }

        Ingredient ingredient{*id, row[1], row[2], *price, *grams, row[5]};
        if (!ingredientsById_.emplace(*id, ingredient).second) {
            return fail("ingredients.csv contains a duplicate ingredient ID.");
        }
    }

    CsvFile::Table recipeIngredients;
    if (!loadTable(
            seedDirectory,
            "recipe_ingredients.csv",
            {"recipe_id", "recipe_name", "ingredient_id", "ingredient_name", "quantity", "unit", "weight_gram", "source_ingredient_text"},
            recipeIngredients,
            error)) {
        return fail(error);
    }
    for (const Row& row : recipeIngredients.rows) {
        const auto recipeId = integer(row[0]);
        const auto ingredientId = integer(row[2]);
        const auto quantity = real(row[4]);
        const auto weight = real(row[6]);

        if (!recipeId || !ingredientId || !quantity || (!row[6].empty() && !weight)) {
            return fail("recipe_ingredients.csv contains an invalid or unlinked record.");
        }
        if (!recipesById_.count(*recipeId) || !ingredientsById_.count(*ingredientId)) {
            return fail("recipe_ingredients.csv contains an invalid or unlinked record.");
        }

        RecipeIngredient recipeIngredient{
            *recipeId,
            *ingredientId,
            row[3],
            *quantity,
            row[5],
            weight,
            row[7]
        };
        recipeIngredientsByRecipe_[*recipeId].push_back(recipeIngredient);
    }

    CsvFile::Table instructions;
    if (!loadTable(
            seedDirectory,
            "recipe_instructions.csv",
            {"recipe_id", "preparation_instructions"},
            instructions,
            error)) {
        return fail(error);
    }
    for (const Row& row : instructions.rows) {
        const auto recipeId = integer(row[0]);
        if (!recipeId || !recipesById_.count(*recipeId)) {
            return fail("recipe_instructions.csv contains an invalid or duplicate record.");
        }
        if (!instructionsByRecipe_.emplace(*recipeId, row[1]).second) {
            return fail("recipe_instructions.csv contains an invalid or duplicate record.");
        }
    }

    CsvFile::Table seasoners;
    if (!loadTable(
            seedDirectory,
            "seasoner.csv",
            {"id", "name", "source_examples"},
            seasoners,
            error)) {
        return fail(error);
    }
    std::unordered_map<std::string, int> seasonerIdsByName;
    for (const Row& row : seasoners.rows) {
        const auto id = integer(row[0]);
        if (!id || seasonersById_.count(*id) || seasonerIdsByName.count(row[1])) {
            return fail("seasoner.csv contains an invalid or duplicate record.");
        }

        seasonersById_[*id] = Seasoner{*id, row[1], row[2]};
        seasonerIdsByName[row[1]] = *id;
    }

    CsvFile::Table recipeSeasoners;
    if (!loadTable(
            seedDirectory,
            "recipes_seasoner.csv",
            {"recipe_id", "recipe_name", "seasoners"},
            recipeSeasoners,
            error)) {
        return fail(error);
    }
    for (const Row& row : recipeSeasoners.rows) {
        const auto recipeId = integer(row[0]);
        if (!recipeId || !recipesById_.count(*recipeId)) {
            return fail("recipes_seasoner.csv contains an invalid or duplicate record.");
        }
        std::vector<int> seasonerIds;
        for (const std::string& name : splitSeasoners(row[2])) {
            const auto found = seasonerIdsByName.find(name);
            if (found == seasonerIdsByName.end()) {
                return fail("recipes_seasoner.csv contains an unknown seasoner.");
            }
            seasonerIds.push_back(found->second);
        }

        if (!seasonersByRecipe_.emplace(*recipeId, seasonerIds).second) {
            return fail("recipes_seasoner.csv contains an invalid or duplicate record.");
        }
    }

    CsvFile::Table dietaryTags;
    if (!loadTable(
            seedDirectory,
            "dietary_tags.csv",
            {"id", "code", "display_name", "tag_group", "description"},
            dietaryTags,
            error)) {
        return fail(error);
    }
    for (const Row& row : dietaryTags.rows) {
        const auto id = integer(row[0]);
        if (!id || !dietaryTagsById_.emplace(*id, DietaryTag{*id, row[1], row[2], row[3], row[4]}).second) {
            return fail("dietary_tags.csv contains an invalid or duplicate record.");
        }
    }

    CsvFile::Table allergens;
    if (!loadTable(
            seedDirectory,
            "allergens.csv",
            {"id", "code", "display_name"},
            allergens,
            error)) {
        return fail(error);
    }
    for (const Row& row : allergens.rows) {
        const auto id = integer(row[0]);
        if (!id || !allergensById_.emplace(*id, Allergen{*id, row[1], row[2]}).second) {
            return fail("allergens.csv contains an invalid or duplicate record.");
        }
    }

    CsvFile::Table ingredientAllergens;
    if (!loadTable(
            seedDirectory,
            "ingredient_allergens.csv",
            {"ingredient_id", "ingredient_name", "allergen_id", "allergen_name", "status", "note"},
            ingredientAllergens,
            error)) {
        return fail(error);
    }
    for (const Row& row : ingredientAllergens.rows) {
        const auto ingredientId = integer(row[0]);
        const auto allergenId = integer(row[2]);
        if (!ingredientId || !allergenId) {
            return fail("ingredient_allergens.csv contains an invalid linked record.");
        }
        if (!ingredientsById_.count(*ingredientId) || !allergensById_.count(*allergenId)) {
            return fail("ingredient_allergens.csv contains an invalid linked record.");
        }
        if (row[4] != "unknown") {
            allergensByIngredient_[*ingredientId].insert(*allergenId);
        }
    }

    CsvFile::Table ingredientDietaryTags;
    if (!loadTable(
            seedDirectory,
            "ingredient_dietary_tags.csv",
            {"ingredient_id", "ingredient_name", "dietary_tag_id", "dietary_tag_name", "status", "note"},
            ingredientDietaryTags,
            error)) {
        return fail(error);
    }
    for (const Row& row : ingredientDietaryTags.rows) {
        const auto ingredientId = integer(row[0]);
        const auto tagId = integer(row[2]);
        if (!ingredientId || !tagId) {
            return fail("ingredient_dietary_tags.csv contains an invalid linked record.");
        }
        if (!ingredientsById_.count(*ingredientId) || !dietaryTagsById_.count(*tagId)) {
            return fail("ingredient_dietary_tags.csv contains an invalid linked record.");
        }
        if (row[4] == "incompatible" || row[4] == "conditional") {
            dietaryConflictsByIngredient_[*ingredientId].insert(*tagId);
        }
    }

    CsvFile::Table seasonerAllergens;
    if (!loadTable(
            seedDirectory,
            "seasoner_allergens.csv",
            {"seasoner_id", "seasoner_name", "allergen_id", "allergen_name", "note"},
            seasonerAllergens,
            error)) {
        return fail(error);
    }
    for (const Row& row : seasonerAllergens.rows) {
        const auto seasonerId = integer(row[0]);
        const auto allergenId = integer(row[2]);
        if (!seasonerId || !allergenId) {
            return fail("seasoner_allergens.csv contains an invalid linked record.");
        }
        if (!seasonersById_.count(*seasonerId) || !allergensById_.count(*allergenId)) {
            return fail("seasoner_allergens.csv contains an invalid linked record.");
        }
        allergensBySeasoner_[*seasonerId].insert(*allergenId);
    }

    CsvFile::Table seasonerDietaryTags;
    if (!loadTable(
            seedDirectory,
            "seasoner_dietary_tags.csv",
            {"seasoner_id", "seasoner_name", "dietary_tag_id", "dietary_tag_name", "note"},
            seasonerDietaryTags,
            error)) {
        return fail(error);
    }
    for (const Row& row : seasonerDietaryTags.rows) {
        const auto seasonerId = integer(row[0]);
        const auto tagId = integer(row[2]);
        if (!seasonerId || !tagId) {
            return fail("seasoner_dietary_tags.csv contains an invalid linked record.");
        }
        if (!seasonersById_.count(*seasonerId) || !dietaryTagsById_.count(*tagId)) {
            return fail("seasoner_dietary_tags.csv contains an invalid linked record.");
        }
        dietaryConflictsBySeasoner_[*seasonerId].insert(*tagId);
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
    if (found == recipesById_.end()) {
        return std::nullopt;
    }
    return found->second;
}

std::vector<Recipe> RecipeDataBase::searchRecipes(const RecipeFilter& filter) const {
    std::vector<Recipe> result;
    std::string titleNeedle;
    if (filter.titleContains) {
        titleNeedle = lower(*filter.titleContains);
    }

    for (const auto& entry : recipesById_) {
        const Recipe& recipe = entry.second;

        if (!titleNeedle.empty() && lower(recipe.title).find(titleNeedle) == std::string::npos) {
            continue;
        }

        if (!filter.mealTypes.empty()) {
            const auto mealType = std::find(
                filter.mealTypes.begin(),
                filter.mealTypes.end(),
                recipe.mealType
            );
            if (mealType == filter.mealTypes.end()) {
                continue;
            }
        }

        if (filter.difficulty && recipe.difficulty != *filter.difficulty) {
            continue;
        }

        if (filter.maximumPrepMinutes) {
            if (!recipe.prepMinutes || *recipe.prepMinutes > *filter.maximumPrepMinutes) {
                continue;
            }
        }

        if (filter.maximumCookMinutes) {
            if (!recipe.cookMinutes || *recipe.cookMinutes > *filter.maximumCookMinutes) {
                continue;
            }
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
    if (found == ingredientsById_.end()) {
        return std::nullopt;
    }
    return found->second;
}

std::vector<Ingredient> RecipeDataBase::getAllIngredients() const {
    std::vector<Ingredient> result;
    for (const auto& entry : ingredientsById_) {
        result.push_back(entry.second);
    }
    std::sort(result.begin(), result.end(), [](const Ingredient& left, const Ingredient& right) {
        return left.name < right.name;
    });
    return result;
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
    if (found == recipeIngredientsByRecipe_.end()) {
        return {};
    }
    return found->second;
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
        result.push_back({
            recipeIngredient.recipeId,
            ingredient.ingredientId,
            ingredient.name,
            recipeIngredient.weightGrams,
            ingredient.pricePer100Grams,
            ingredient.purchaseUnitGrams,
            ingredient.purchaseUnitLabel
        });
    }
    return result;
}

std::optional<std::string> RecipeDataBase::getPreparationInstructions(int recipeId) const {
    const auto found = instructionsByRecipe_.find(recipeId);
    if (found == instructionsByRecipe_.end()) {
        return std::nullopt;
    }
    return found->second;
}

std::vector<std::string> RecipeDataBase::getRecipeSeasoners(int recipeId) const {
    const auto found = seasonersByRecipe_.find(recipeId);
    if (found == seasonersByRecipe_.end()) {
        return {};
    }
    std::vector<std::string> names;
    names.reserve(found->second.size());
    for (const int seasonerId : found->second) {
        names.push_back(seasonersById_.at(seasonerId).name);
    }
    return names;
}

std::vector<DietaryTag> RecipeDataBase::getDietaryTags() const {
    std::vector<DietaryTag> result;
    for (const auto& entry : dietaryTagsById_) {
        result.push_back(entry.second);
    }
    std::sort(result.begin(), result.end(), [](const DietaryTag& left, const DietaryTag& right) {
        return left.dietaryTagId < right.dietaryTagId;
    });
    return result;
}

std::vector<Allergen> RecipeDataBase::getAllergens() const {
    std::vector<Allergen> result;
    for (const auto& entry : allergensById_) {
        result.push_back(entry.second);
    }
    std::sort(result.begin(), result.end(), [](const Allergen& left, const Allergen& right) {
        return left.displayName < right.displayName;
    });
    return result;
}

std::vector<Recipe> RecipeDataBase::findCompatibleRecipes(
    const std::vector<int>& dietaryTagIds,
    const std::vector<int>& allergenIds
) const {
    std::unordered_set<int> requestedDietaryTags(dietaryTagIds.begin(), dietaryTagIds.end());
    std::unordered_set<int> requestedAllergens(allergenIds.begin(), allergenIds.end());
    std::vector<Recipe> result;

    // A recipe is removed if one ingredient or seasoner has a selected conflict.
    for (const auto& recipeEntry : recipesById_) {
        bool compatible = true;

        const auto ingredients = recipeIngredientsByRecipe_.find(recipeEntry.first);
        if (ingredients != recipeIngredientsByRecipe_.end()) {
            for (const RecipeIngredient& recipeIngredient : ingredients->second) {
                const auto dietary = dietaryConflictsByIngredient_.find(recipeIngredient.ingredientId);
                if (dietary != dietaryConflictsByIngredient_.end() &&
                    containsAny(dietary->second, requestedDietaryTags)) {
                    compatible = false;
                    break;
                }

                const auto allergens = allergensByIngredient_.find(recipeIngredient.ingredientId);
                if (allergens != allergensByIngredient_.end() &&
                    containsAny(allergens->second, requestedAllergens)) {
                    compatible = false;
                    break;
                }
            }
        }

        const auto recipeSeasoners = seasonersByRecipe_.find(recipeEntry.first);
        if (compatible && recipeSeasoners != seasonersByRecipe_.end()) {
            for (int seasonerId : recipeSeasoners->second) {
                const auto dietary = dietaryConflictsBySeasoner_.find(seasonerId);
                if (dietary != dietaryConflictsBySeasoner_.end() &&
                    containsAny(dietary->second, requestedDietaryTags)) {
                    compatible = false;
                    break;
                }

                const auto allergens = allergensBySeasoner_.find(seasonerId);
                if (allergens != allergensBySeasoner_.end() &&
                    containsAny(allergens->second, requestedAllergens)) {
                    compatible = false;
                    break;
                }
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
    seasonersById_.clear();
    seasonersByRecipe_.clear();
    dietaryTagsById_.clear();
    allergensById_.clear();
    dietaryConflictsByIngredient_.clear();
    allergensByIngredient_.clear();
    dietaryConflictsBySeasoner_.clear();
    allergensBySeasoner_.clear();
    loaded_ = false;
    lastError_.clear();
}
