add_executable(BudgetBites
        main.cpp
        Account.cpp
        Grocery.cpp
        MealPlan.cpp
        RecipeDatabase.cpp
        Infrastructure.cpp
        UX.cpp
)

target_include_directories(BudgetBites PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)