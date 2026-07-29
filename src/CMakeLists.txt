add_library(BudgetBitesLib
        Account.cpp
        Grocery.cpp
        Infrastructure.cpp
        MealPlan.cpp
        RecipeDataBase.cpp
        UX.cpp
        PasswordSecurity.cpp
        UserRepository.cpp
        ProfileImageStore.cpp
        Preferences.cpp

)

target_include_directories(BudgetBitesLib
        PUBLIC ${CMAKE_SOURCE_DIR}/include
)

add_executable(BudgetBites
        Main.cpp
        AllergyTest.cpp
        AllergyTest.h
        Preferences.cpp
        ../include/BudgetBitesLib/Preferences.h
)

target_link_libraries(BudgetBites
        PRIVATE BudgetBitesLib
)