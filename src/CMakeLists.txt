add_library(BudgetBitesLib
        Account.cpp
        CsvFile.cpp
        Grocery.cpp
        Infrastructure.cpp
        MainHelper.cpp
        MealPlan.cpp
        RecipeDataBase.cpp
        UX.cpp
        PasswordSecurity.cpp
        UserInfoRepository.cpp
        UserRepository.cpp
        ProfileImageStore.cpp
        Preferences.cpp
        MealGenerator.cpp
        ../include/BudgetBitesLib/MealGenerator.h
        Ingredients.cpp
        ../include/BudgetBitesLib/Ingredients.h

)

target_include_directories(BudgetBitesLib
        PUBLIC ${CMAKE_SOURCE_DIR}/include
)

add_executable(BudgetBites
        Main.cpp
)

target_link_libraries(BudgetBites
        PRIVATE BudgetBitesLib
)
