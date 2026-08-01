#include "BudgetBitesLib/UserInfoRepository.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <random>
#include "TestUtils.h"

TEST_CASE("UserInfoRepository persists supplemental user information", "[userinfo]") {
    const auto directory = makeUniqueTestDir("budgetbites-user-info-test");
    std::filesystem::remove_all(directory);

    UserInfoRepository repository(directory);
    REQUIRE(repository.ensureUser("student"));
    REQUIRE(repository.ensureUser("STUDENT"));
    REQUIRE(repository.updateWeeklyBudget("student", 45.0));
    REQUIRE(repository.replaceDietaryTagIds("student", {1, 4, 1}));
    REQUIRE(repository.replaceAllergenIds("student", {1}));
    REQUIRE(repository.replacePantryItems("student", {{22, 300.0}, {153, std::nullopt}}));
    REQUIRE(repository.save());

    UserInfoRepository reloaded(directory);
    const auto savedUser = reloaded.getUserByUsername("STUDENT");
    REQUIRE(savedUser.has_value());
    CHECK(savedUser->username == "student");
    CHECK(savedUser->weeklyBudget == 45.0);
    CHECK(reloaded.getDietaryTagIds("STUDENT") == std::vector<int>{1, 4});
    CHECK(reloaded.getAllergenIds("STUDENT") == std::vector<int>{1});
    const auto pantryItems = reloaded.getPantryItems("STUDENT");
    REQUIRE(pantryItems.size() == 2);
    REQUIRE(pantryItems[0].availableGrams.has_value());
    CHECK(*pantryItems[0].availableGrams == 300.0);
    CHECK_FALSE(pantryItems[1].availableGrams.has_value());

    std::filesystem::remove_all(directory);
}
