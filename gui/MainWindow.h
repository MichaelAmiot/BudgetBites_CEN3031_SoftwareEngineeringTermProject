#pragma once

#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Grocery.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/MealGenerator.h"
#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/RecipeDataBase.h"
#include "BudgetBitesLib/UX.h"

#include <QMainWindow>

#include <filesystem>
#include <optional>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTextEdit;
class QWidget;
class QCloseEvent;

class DashboardPage;
class AccountPage;
class PreferencesPage;
class MealPlanPage;
class RecipePage;
class GroceryPage;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::MainWindow* ui = nullptr;

    DashboardPage* dashboardPageWidget_ = nullptr;
    AccountPage* accountPageWidget_ = nullptr;
    PreferencesPage* preferencesPageWidget_ = nullptr;
    MealPlanPage* mealPlanPageWidget_ = nullptr;
    RecipePage* recipePageWidget_ = nullptr;
    GroceryPage* groceryPageWidget_ = nullptr;

    enum PageIndex {
        DashboardPageIndex = 0,
        AccountPageIndex,
        PreferencesPageIndex,
        MealPlanPageIndex,
        RecipePageIndex,
        GroceryPageIndex
    };

    std::filesystem::path projectRoot_;
    std::filesystem::path usersFile_;

    UX ux_;
    Account account_;
    Preferences preferences_;
    Ingredients pantry_;
    MealPlan mealPlan_;
    MealGenerator mealGenerator_;
    Grocery grocery_;
    RecipeDataBase catalog_;

    QStackedWidget* pages_ = nullptr;
    QLabel* signedInLabel_ = nullptr;
    QLabel* globalStatusLabel_ = nullptr;

    // Account page
    QLineEdit* usernameEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QLabel* accountStatusLabel_ = nullptr;
    QPushButton* signOutButton_ = nullptr;

    // Preferences page
    QDoubleSpinBox* budgetSpin_ = nullptr;
    QListWidget* dietaryList_ = nullptr;
    QListWidget* allergenList_ = nullptr;
    QTableWidget* pantryTable_ = nullptr;
    QLabel* preferencesStatusLabel_ = nullptr;

    // Meal-plan page
    QComboBox* generationModeCombo_ = nullptr;
    QTableWidget* mealPlanTable_ = nullptr;
    QLabel* mealPlanSummaryLabel_ = nullptr;

    // Recipe page
    QLineEdit* recipeSearchEdit_ = nullptr;
    QListWidget* recipeResultsList_ = nullptr;
    QTextEdit* recipeDetailsText_ = nullptr;
    std::vector<Recipe> currentRecipeResults_;

    // Grocery page
    QTableWidget* groceryTable_ = nullptr;
    QLabel* grocerySummaryLabel_ = nullptr;


    void applyStyle();
    void showPage(PageIndex page);
    void setGlobalStatus(const QString& message, bool error = false);
    void updateSignedInState();

    void registerUser();
    void checkUsernameAvailability();
    void signIn();
    void signOut();

    void populatePreferenceControls();
    void loadPreferenceControlsFromState();
    void savePreferences();

    void generateMealPlan();
    void refreshMealPlanTable();
    void refreshGroceryTable();

    void searchRecipes();
    void displaySelectedRecipe();
    QString recipeDetails(const Recipe& recipe) const;

    std::vector<int> checkedIds(const QListWidget* list) const;
    void setCheckedIds(QListWidget* list, const std::vector<int>& ids);
    std::vector<PantryItem> pantryItemsFromTable() const;
    void setPantryTableItems(const std::vector<PantryItem>& items);

    bool requireSignIn(const QString& action);
};
