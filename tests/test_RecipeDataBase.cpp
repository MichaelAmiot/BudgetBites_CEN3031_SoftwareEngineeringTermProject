#include "BudgetBitesLib/RecipeDataBase.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>

namespace {

std::filesystem::path catalogPath() {
    return std::filesystem::path(BUDGETBITES_SOURCE_DIR) / "data" / "seed";
}

} // namespace

TEST_CASE("RecipeDataBase reads recipe details from the CSV catalog", "[recipedatabase]") {
    RecipeDataBase database(catalogPath());
    REQUIRE(database.isLoaded());

    const auto recipe = database.getRecipeById(1);
    REQUIRE(recipe.has_value());
    CHECK(recipe->title == "20-Minute Beef Stroganoff");

    const auto ingredients = database.getRecipeIngredients(1);
    REQUIRE_FALSE(ingredients.empty());
    CHECK(ingredients.front().ingredientName == "Pasta");

    const auto instructions = database.getPreparationInstructions(1);
    REQUIRE(instructions.has_value());
    CHECK(instructions->find("Prepare noodles") != std::string::npos);

    const auto seasoners = database.getRecipeSeasoners(1);
    CHECK(std::find(seasoners.begin(), seasoners.end(), "Salt") != seasoners.end());
}

TEST_CASE("RecipeDataBase applies filters and excludes dietary conflicts", "[recipedatabase]") {
    RecipeDataBase database(catalogPath());
    REQUIRE(database.isLoaded());

    RecipeFilter filter;
    filter.titleContains = "curry";
    filter.maximumCookMinutes = 40;
    const auto searched = database.searchRecipes(filter);
    REQUIRE_FALSE(searched.empty());
    CHECK(std::all_of(searched.begin(), searched.end(), [](const Recipe& recipe) {
        return recipe.cookMinutes.has_value() && *recipe.cookMinutes <= 40;
    }));

    const auto dairyFree = database.findCompatibleRecipes({4}, {});
    CHECK(std::none_of(dairyFree.begin(), dairyFree.end(), [](const Recipe& recipe) {
        return recipe.recipeId == 1;
    }));

    const auto milkFree = database.findCompatibleRecipes({}, {1});
    CHECK(std::none_of(milkFree.begin(), milkFree.end(), [](const Recipe& recipe) {
        return recipe.recipeId == 1;
    }));

    CHECK(database.getDietaryTags().size() == 26);
    CHECK(database.getAllergens().size() == 15);
}
