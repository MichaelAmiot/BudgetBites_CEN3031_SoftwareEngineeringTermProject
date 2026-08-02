add_executable(BudgetBitesTests
        test_account.cpp
        test_grocery.cpp
        test_MainHelper.cpp
        test_MealPlan.cpp
        test_RecipeDataBase.cpp
        test_UX.cpp
        test_PasswordSecurity.cpp
        test_ProfileImageStore.cpp
        test_UserInfoRepository.cpp
        test_Preferences.cpp
        test_MealGenerator.cpp
        test_ingredients.cpp
        TestUtils.h
)

target_link_libraries(BudgetBitesTests
        PRIVATE
        BudgetBitesLib
        Catch2::Catch2WithMain
)

target_include_directories(BudgetBitesTests
        PRIVATE ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(BudgetBitesTests
        PRIVATE BUDGETBITES_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

include(CTest)
include(Catch)
catch_discover_tests(BudgetBitesTests)
