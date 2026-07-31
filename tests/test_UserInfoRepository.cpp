#include "BudgetBitesLib/UserInfoRepository.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

TEST_CASE("UserInfoRepository persists profile preferences and pantry items", "[userinfo]") {
    const auto directory = std::filesystem::temp_directory_path() / "budgetbites-user-info-test";
    std::filesystem::remove_all(directory);

    UserInfoRepository repository(directory);
    const auto user = repository.createUser("student", "password-hash", "password-salt");
    REQUIRE(user.has_value());
    REQUIRE(repository.updateWeeklyBudget(user->id, 45.0));
    REQUIRE(repository.replaceDietaryTagIds(user->id, {1, 4}));
    REQUIRE(repository.replaceAllergenIds(user->id, {1}));
    REQUIRE(repository.replacePantryItems(user->id, {{22, 300.0}, {153, std::nullopt}}));
    REQUIRE(repository.save());

    UserInfoRepository reloaded(directory);
    const auto savedUser = reloaded.getUserByUsername("STUDENT");
    REQUIRE(savedUser.has_value());
    CHECK(savedUser->weeklyBudget == 45.0);
    CHECK(reloaded.getDietaryTagIds(savedUser->id) == std::vector<int>{1, 4});
    CHECK(reloaded.getAllergenIds(savedUser->id) == std::vector<int>{1});
    CHECK(reloaded.getPantryItems(savedUser->id).size() == 2);

    std::filesystem::remove_all(directory);
}
