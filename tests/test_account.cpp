#include "BudgetBitesLib/Account.h"

#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE("Account keeps the original allergy text functions", "[account]") {
    Account account;
    account.setAllergies({"Milk", "Peanut"});

    CHECK(account.getAllergies() == std::vector<std::string>{"Milk", "Peanut"});
}

TEST_CASE("Account stores catalog allergen IDs", "[account]") {
    Account account;
    account.setAllergenIds({1, 6});

    CHECK(account.getAllergenIds() == std::vector<int>{1, 6});
}

TEST_CASE("Account lets the user select from available allergens", "[account]") {
    Account account;
    const std::vector<Allergen> available = {
        {1, "milk", "Milk"},
        {2, "egg", "Egg"},
        {6, "peanut", "Peanut"}
    };

    std::istringstream input("1\n6\n0\n");
    std::ostringstream output;
    std::streambuf* originalInput = std::cin.rdbuf(input.rdbuf());
    std::streambuf* originalOutput = std::cout.rdbuf(output.rdbuf());
    account.enterFoodAllergies(available);
    std::cin.rdbuf(originalInput);
    std::cout.rdbuf(originalOutput);

    CHECK(account.getAllergenIds() == std::vector<int>{1, 6});
    CHECK(output.str().find("Milk") != std::string::npos);
    CHECK(output.str().find("Peanut") != std::string::npos);
}
