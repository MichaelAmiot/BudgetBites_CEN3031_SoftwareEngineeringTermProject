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

// The one and only window for the desktop app. Holds every page (dashboard,
//  account, preferences, meal plan, recipes, grocery) in a QStackedWidget and
//  switches between them, and owns the actual business objects (UX, Account,
//  Preferences, and so on) that the console app also uses under the hood.
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    // Builds every page, wires up all the buttons, and applies the dark theme.
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    // Saves whatever the signed in user has set before the window actually closes.
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::MainWindow* ui = nullptr;

    // Each of these is one screen of the app. They live inside the pages_
    //  stacked widget below and only one is visible at a time.
    DashboardPage* dashboardPageWidget_ = nullptr;
    AccountPage* accountPageWidget_ = nullptr;
    PreferencesPage* preferencesPageWidget_ = nullptr;
    MealPlanPage* mealPlanPageWidget_ = nullptr;
    RecipePage* recipePageWidget_ = nullptr;
    GroceryPage* groceryPageWidget_ = nullptr;

    // Matches the order the pages were added to pages_, so showPage can
    //  just flip to the right index instead of comparing widget pointers.
    enum PageIndex {
        DashboardPageIndex = 0,
        AccountPageIndex,
        PreferencesPageIndex,
        MealPlanPageIndex,
        RecipePageIndex,
        GroceryPageIndex
    };

    // Where the account data lives on disk for this run of the app.
    std::filesystem::path projectRoot_;
    std::filesystem::path usersFile_;

    // These are the same business objects the console app works with.
    // The GUI just gives them buttons and text fields instead of a menu.
    UX ux_;
    Account account_;
    Preferences preferences_;
    Ingredients pantry_;
    MealPlan mealPlan_;
    MealGenerator mealGenerator_;
    Grocery grocery_;
    RecipeDataBase catalog_;

    // Header bar, shown above every page.
    QStackedWidget* pages_ = nullptr;
    QLabel* signedInLabel_ = nullptr;
    QLabel* headerAvatarLabel_ = nullptr;
    QLabel* globalStatusLabel_ = nullptr;

    // Account page
    QLineEdit* usernameEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QLabel* accountStatusLabel_ = nullptr;
    QPushButton* signOutButton_ = nullptr;
    QLabel* profileImagePreview_ = nullptr;
    QPushButton* uploadPhotoButton_ = nullptr;
    QPushButton* removePhotoButton_ = nullptr;

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


    // Sets up the dark color theme used across the whole app.
    void applyStyle();
    // Switches which page is currently visible.
    void showPage(PageIndex page);
    // Updates the small status line at the bottom of the window.
    void setGlobalStatus(const QString& message, bool error = false);
    // Refreshes everything that depends on whether someone is signed in:
    //  the header text, the avatar photo, and which account buttons are enabled.
    void updateSignedInState();

    // Account page actions
    void registerUser();
    void checkUsernameAvailability();
    void signIn();
    void signOut();
    void uploadProfilePhoto();
    void removeProfilePhoto();
    void refreshProfileImagePreview();

    // Preferences page actions
    void populatePreferenceControls();
    void loadPreferenceControlsFromState();
    void savePreferences();
    // Unchecks every row in the pantry table and clears its gram amounts.
    // Does not touch the budget, dietary, or allergen controls, and does
    //  not save to disk until Save Preferences is clicked afterward.
    void clearPantryTable();

    // Meal plan page actions
    void generateMealPlan();
    // Clears the currently generated weekly meal plan and grocery list back
    //  to empty, without touching saved budget/dietary/allergen/pantry
    //  preferences.
    void clearMealPlan();
    void refreshMealPlanTable();
    void refreshGroceryTable();

    // Recipe page actions
    void searchRecipes();
    void displaySelectedRecipe();
    QString recipeDetails(const Recipe& recipe) const;

    // Small helpers used by the preferences and pantry controls.
    std::vector<int> checkedIds(const QListWidget* list) const;
    void setCheckedIds(QListWidget* list, const std::vector<int>& ids);
    std::vector<PantryItem> pantryItemsFromTable() const;
    void setPantryTableItems(const std::vector<PantryItem>& items);

    // Shows a message and jumps to the account page if nobody is signed in.
    // Returns true if it is fine to continue with whatever action was asked for.
    bool requireSignIn(const QString& action);
};
