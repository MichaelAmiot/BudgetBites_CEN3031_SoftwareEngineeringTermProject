qt_add_executable(BudgetBitesGUI
    GuiMain.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui

    pages/DashboardPage.cpp
    pages/DashboardPage.h
    pages/DashboardPage.ui
    pages/AccountPage.cpp
    pages/AccountPage.h
    pages/AccountPage.ui
    pages/PreferencesPage.cpp
    pages/PreferencesPage.h
    pages/PreferencesPage.ui
    pages/MealPlanPage.cpp
    pages/MealPlanPage.h
    pages/MealPlanPage.ui
    pages/RecipePage.cpp
    pages/RecipePage.h
    pages/RecipePage.ui
    pages/GroceryPage.cpp
    pages/GroceryPage.h
    pages/GroceryPage.ui
)

target_include_directories(BudgetBitesGUI
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/pages
)

target_compile_definitions(BudgetBitesGUI
    PRIVATE
        BUDGETBITES_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(BudgetBitesGUI
    PRIVATE
        BudgetBitesLib
        Qt6::Widgets
)

set_target_properties(BudgetBitesGUI PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/pages"
)
