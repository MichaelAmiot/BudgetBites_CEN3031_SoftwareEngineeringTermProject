#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "pages/DashboardPage.h"
#include "ui_DashboardPage.h"
#include "pages/AccountPage.h"
#include "ui_AccountPage.h"
#include "pages/PreferencesPage.h"
#include "ui_PreferencesPage.h"
#include "pages/MealPlanPage.h"
#include "ui_MealPlanPage.h"
#include "pages/RecipePage.h"
#include "ui_RecipePage.h"
#include "pages/GroceryPage.h"
#include "ui_GroceryPage.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace {
    QString money(double value) {
        return QString("$%1").arg(value, 0, 'f', 2);
    }

    QLabel* makeMutedLabel(const QString& text) {
        auto* label = new QLabel(text);
        label->setObjectName("mutedLabel");
        label->setWordWrap(true);
        return label;
    }

    QTableWidgetItem* readOnlyItem(const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        return item;
    }

    QString optionalNumber(const std::optional<int>& value, const QString& suffix = {}) {
        return value ? QString::number(*value) + suffix : QString("Not provided");
    }
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      projectRoot_(BUDGETBITES_SOURCE_DIR),
      usersFile_(projectRoot_ / UserRepository::defaultStoragePath()),
      ux_(projectRoot_ / "data" / "local"),
      catalog_(projectRoot_ / "data" / "seed") {
    ui->setupUi(this);

    // Each screen is its own Qt Designer form. MainWindow only hosts and
    // coordinates those pages while the existing backend classes remain here.
    dashboardPageWidget_ = new DashboardPage(this);
    accountPageWidget_ = new AccountPage(this);
    preferencesPageWidget_ = new PreferencesPage(this);
    mealPlanPageWidget_ = new MealPlanPage(this);
    recipePageWidget_ = new RecipePage(this);
    groceryPageWidget_ = new GroceryPage(this);

    ui->pages->addWidget(dashboardPageWidget_);
    ui->pages->addWidget(accountPageWidget_);
    ui->pages->addWidget(preferencesPageWidget_);
    ui->pages->addWidget(mealPlanPageWidget_);
    ui->pages->addWidget(recipePageWidget_);
    ui->pages->addWidget(groceryPageWidget_);

    pages_ = ui->pages;
    signedInLabel_ = ui->signedInLabel;
    globalStatusLabel_ = ui->globalStatusLabel;

    auto* dashboardUi = dashboardPageWidget_->ui();
    auto* accountUi = accountPageWidget_->ui();
    auto* preferencesUi = preferencesPageWidget_->ui();
    auto* mealPlanUi = mealPlanPageWidget_->ui();
    auto* recipeUi = recipePageWidget_->ui();
    auto* groceryUi = groceryPageWidget_->ui();

    usernameEdit_ = accountUi->usernameEdit;
    passwordEdit_ = accountUi->passwordEdit;
    accountStatusLabel_ = accountUi->accountStatusLabel;
    signOutButton_ = accountUi->signOutButton;

    budgetSpin_ = preferencesUi->budgetSpin;
    dietaryList_ = preferencesUi->dietaryList;
    allergenList_ = preferencesUi->allergenList;
    pantryTable_ = preferencesUi->pantryTable;
    preferencesStatusLabel_ = preferencesUi->preferencesStatusLabel;

    generationModeCombo_ = mealPlanUi->generationModeCombo;
    mealPlanTable_ = mealPlanUi->mealPlanTable;
    mealPlanSummaryLabel_ = mealPlanUi->mealPlanSummaryLabel;

    recipeSearchEdit_ = recipeUi->recipeSearchEdit;
    recipeResultsList_ = recipeUi->recipeResultsList;
    recipeDetailsText_ = recipeUi->recipeDetailsText;

    groceryTable_ = groceryUi->groceryTable;
    grocerySummaryLabel_ = groceryUi->grocerySummaryLabel;

    pantryTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    pantryTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    pantryTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    pantryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    generationModeCombo_->addItem(
        "Normal — balance variety, reuse, and cost",
        static_cast<int>(MealGenerationMode::Normal)
    );
    generationModeCombo_->addItem(
        "Budget First — complete plan with budget priority",
        static_cast<int>(MealGenerationMode::BudgetFirst)
    );
    generationModeCombo_->addItem(
        "Strict Budget — stop rather than exceed budget",
        static_cast<int>(MealGenerationMode::StrictBudget)
    );

    mealPlanTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mealPlanTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    mealPlanTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    mealPlanTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    mealPlanTable_->verticalHeader()->setVisible(false);
    mealPlanTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mealPlanTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    groceryTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    groceryTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    groceryTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    groceryTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    groceryTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    groceryTable_->verticalHeader()->setVisible(false);
    groceryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(dashboardUi->accountNavigationButton, &QPushButton::clicked, this, [this] {
        showPage(AccountPageIndex);
    });
    connect(dashboardUi->preferencesNavigationButton, &QPushButton::clicked, this, [this] {
        showPage(PreferencesPageIndex);
    });
    connect(dashboardUi->mealPlanNavigationButton, &QPushButton::clicked, this, [this] {
        showPage(MealPlanPageIndex);
    });
    connect(dashboardUi->recipeNavigationButton, &QPushButton::clicked, this, [this] {
        showPage(RecipePageIndex);
    });
    connect(dashboardUi->groceryNavigationButton, &QPushButton::clicked, this, [this] {
        showPage(GroceryPageIndex);
    });

    for (QPushButton* button: {
             accountUi->accountBackButton,
             preferencesUi->preferencesBackButton,
             mealPlanUi->mealPlanBackButton,
             recipeUi->recipeBackButton,
             groceryUi->groceryBackButton
         }) {
        connect(button, &QPushButton::clicked, this, [this] {
            showPage(DashboardPageIndex);
        });
    }

    connect(accountUi->registerButton, &QPushButton::clicked, this, &MainWindow::registerUser);
    connect(accountUi->signInButton, &QPushButton::clicked, this, &MainWindow::signIn);
    connect(accountUi->signOutButton, &QPushButton::clicked, this, &MainWindow::signOut);
    connect(passwordEdit_, &QLineEdit::returnPressed, this, &MainWindow::signIn);

    connect(preferencesUi->savePreferencesButton, &QPushButton::clicked, this, &MainWindow::savePreferences);
    connect(preferencesUi->reloadPreferencesButton, &QPushButton::clicked, this,
            &MainWindow::loadPreferenceControlsFromState);

    connect(mealPlanUi->generateMealPlanButton, &QPushButton::clicked, this, &MainWindow::generateMealPlan);
    connect(mealPlanUi->openGroceryButton, &QPushButton::clicked, this, [this] {
        refreshGroceryTable();
        showPage(GroceryPageIndex);
    });

    connect(recipeUi->recipeSearchButton, &QPushButton::clicked, this, &MainWindow::searchRecipes);
    connect(recipeSearchEdit_, &QLineEdit::returnPressed, this, &MainWindow::searchRecipes);
    connect(recipeResultsList_, &QListWidget::currentRowChanged, this, [this](int) {
        displaySelectedRecipe();
    });

    connect(groceryUi->rebuildGroceryButton, &QPushButton::clicked, this, [this] {
        grocery_.buildFromMealPlan(mealPlan_, catalog_, pantry_);
        refreshGroceryTable();
    });
    connect(groceryUi->openMealPlanButton, &QPushButton::clicked, this, [this] {
        showPage(MealPlanPageIndex);
    });

    ux_.loadFromFile(usersFile_.string());
    applyStyle();
    populatePreferenceControls();
    loadPreferenceControlsFromState();
    updateSignedInState();
    showPage(DashboardPageIndex);

    if (!catalog_.isLoaded()) {
        setGlobalStatus(
            "Recipe database could not be loaded: " + QString::fromStdString(catalog_.lastError()),
            true
        );
    } else {
        setGlobalStatus("Ready. Sign in to save preferences and generate a personalized plan.");
    }
}

MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent* event) {
    ux_.saveToFile(usersFile_.string());
    if (ux_.isSignedIn()) {
        ux_.saveCurrentUserInfo(account_, preferences_, pantry_);
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::applyStyle() {
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #171a1f;
            color: #f5f7fa;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
            font-size: 14px;
        }
        #appHeader {
            background: #101216;
            border-bottom: 1px solid #30343c;
        }
        #brandLabel {
            font-size: 24px;
            font-weight: 750;
        }
        #signedInLabel, #dashboardSubtitle, #heroText, #accountSubtitle, #accountStatusLabel, #preferencesSubtitle, #preferencesStatusLabel, #mealPlanSubtitle, #mealPlanSummaryLabel, #recipeSubtitle, #grocerySubtitle, #grocerySummaryLabel, #globalStatusLabel {
            color: #aeb6c2;
        }
        #globalStatusLabel {
            background: #101216;
            border-top: 1px solid #30343c;
        }
        #dashboardTitle, #accountTitle, #preferencesTitle, #mealPlanTitle, #recipeTitle, #groceryTitle {
            font-size: 28px;
            font-weight: 750;
        }
        #heroCard, #accountCard, #budgetCard {
            background: #20242b;
            border: 1px solid #343a45;
            border-radius: 12px;
        }
        #heroTitle {
            font-size: 22px;
            font-weight: 700;
        }
        QPushButton {
            background: #2f7df6;
            border: none;
            border-radius: 7px;
            padding: 10px 16px;
            color: white;
            font-weight: 650;
        }
        QPushButton:hover { background: #438cf7; }
        QPushButton:pressed { background: #246bd8; }
        QPushButton:disabled { background: #4a4f59; color: #9ba2ad; }
        #accountNavigationButton, #preferencesNavigationButton, #mealPlanNavigationButton, #recipeNavigationButton, #groceryNavigationButton {
            background: #242932;
            border: 1px solid #3b424e;
            font-size: 16px;
            text-align: left;
            padding-left: 22px;
        }
        #accountNavigationButton:hover, #preferencesNavigationButton:hover, #mealPlanNavigationButton:hover, #recipeNavigationButton:hover, #groceryNavigationButton:hover { background: #2b323d; border-color: #4f8ff7; }
        #signOutButton, #accountBackButton, #reloadPreferencesButton, #preferencesBackButton, #openGroceryButton, #mealPlanBackButton, #recipeBackButton, #openMealPlanButton, #groceryBackButton {
            background: #2a2f37;
            border: 1px solid #454c58;
        }
        QLineEdit, QDoubleSpinBox, QComboBox, QListWidget, QTableWidget, QTextEdit {
            background: #20242b;
            border: 1px solid #3a414c;
            border-radius: 6px;
            padding: 7px;
            selection-background-color: #2f7df6;
        }
        QHeaderView::section {
            background: #292f38;
            color: #f5f7fa;
            border: none;
            border-right: 1px solid #3b424d;
            padding: 9px;
            font-weight: 650;
        }
        QTableWidget { gridline-color: #333944; }
        QScrollBar:vertical { background: #171a1f; width: 12px; }
        QScrollBar::handle:vertical { background: #444b57; border-radius: 5px; min-height: 24px; }
    )");
}

void MainWindow::showPage(PageIndex page) {
    if (page == PreferencesPageIndex) {
        loadPreferenceControlsFromState();
    } else if (page == MealPlanPageIndex) {
        refreshMealPlanTable();
    } else if (page == RecipePageIndex && currentRecipeResults_.empty()) {
        searchRecipes();
    } else if (page == GroceryPageIndex) {
        refreshGroceryTable();
    }
    pages_->setCurrentIndex(page);
}

void MainWindow::setGlobalStatus(const QString& message, bool error) {
    globalStatusLabel_->setText(message);
    globalStatusLabel_->setStyleSheet(error ? "color: #ff8f8f;" : "color: #aeb6c2;");
}

void MainWindow::updateSignedInState() {
    if (ux_.isSignedIn()) {
        const QString username = QString::fromStdString(*ux_.currentUser());
        signedInLabel_->setText("Signed in as " + username);
        accountStatusLabel_->setText("Signed in as " + username + ". Your saved information is loaded.");
        signOutButton_->setEnabled(true);
    } else {
        signedInLabel_->setText("Not signed in");
        accountStatusLabel_->setText(
            "Not signed in. You may browse recipes, but sign in to save personalized information.");
        signOutButton_->setEnabled(false);
    }
}

void MainWindow::registerUser() {
    const std::string username = usernameEdit_->text().trimmed().toStdString();
    const std::string password = passwordEdit_->text().toStdString();
    if (!ux_.registerUser(username, password)) {
        accountStatusLabel_->setText("Registration failed. The username may exist or the password may be too weak.");
        return;
    }
    ux_.saveToFile(usersFile_.string());
    accountStatusLabel_->setText("Registration succeeded. You can now sign in.");
    passwordEdit_->clear();
}

void MainWindow::signIn() {
    const std::string username = usernameEdit_->text().trimmed().toStdString();
    const std::string password = passwordEdit_->text().toStdString();
    if (!ux_.signIn(username, password)) {
        accountStatusLabel_->setText("Sign-in failed. Check the username and password.");
        return;
    }

    account_ = Account{};
    preferences_ = Preferences{};
    pantry_ = Ingredients{};
    mealPlan_ = MealPlan{};
    grocery_ = Grocery{};
    ux_.loadCurrentUserInfo(account_, preferences_, pantry_);
    passwordEdit_->clear();
    loadPreferenceControlsFromState();
    refreshMealPlanTable();
    refreshGroceryTable();
    updateSignedInState();
    setGlobalStatus("Signed in successfully.");
}

void MainWindow::signOut() {
    if (ux_.isSignedIn()) {
        ux_.saveCurrentUserInfo(account_, preferences_, pantry_);
    }
    ux_.signOut();
    account_ = Account{};
    preferences_ = Preferences{};
    pantry_ = Ingredients{};
    mealPlan_ = MealPlan{};
    grocery_ = Grocery{};
    loadPreferenceControlsFromState();
    refreshMealPlanTable();
    refreshGroceryTable();
    updateSignedInState();
    setGlobalStatus("Signed out.");
}

void MainWindow::populatePreferenceControls() {
    dietaryList_->clear();
    for (const DietaryTag& tag: catalog_.getDietaryTags()) {
        auto* item = new QListWidgetItem(QString::fromStdString(tag.displayName));
        item->setData(Qt::UserRole, tag.dietaryTagId);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setToolTip(QString::fromStdString(tag.description));
        dietaryList_->addItem(item);
    }

    allergenList_->clear();
    for (const Allergen& allergen: catalog_.getAllergens()) {
        auto* item = new QListWidgetItem(QString::fromStdString(allergen.displayName));
        item->setData(Qt::UserRole, allergen.allergenId);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        allergenList_->addItem(item);
    }

    const auto ingredients = catalog_.getAllIngredients();
    pantryTable_->setRowCount(static_cast<int>(ingredients.size()));
    for (int row = 0; row < static_cast<int>(ingredients.size()); ++row) {
        const Ingredient& ingredient = ingredients[static_cast<std::size_t>(row)];
        auto* check = new QCheckBox;
        check->setProperty("ingredientId", ingredient.ingredientId);
        pantryTable_->setCellWidget(row, 0, check);
        pantryTable_->setItem(row, 1, readOnlyItem(QString::fromStdString(ingredient.name)));

        auto* grams = new QDoubleSpinBox;
        grams->setRange(0.0, 100000.0);
        grams->setDecimals(1);
        grams->setSuffix(" g");
        grams->setSpecialValueText("Unknown");
        grams->setEnabled(false);
        pantryTable_->setCellWidget(row, 2, grams);
        connect(check, &QCheckBox::toggled, grams, &QWidget::setEnabled);
    }
}

void MainWindow::loadPreferenceControlsFromState() {
    budgetSpin_->setValue(preferences_.getBudget());
    setCheckedIds(dietaryList_, preferences_.getDietaryTagIds());
    setCheckedIds(allergenList_, account_.getAllergenIds());
    setPantryTableItems(pantry_.getPantryItems());
}

void MainWindow::savePreferences() {
    preferences_.setBudget(budgetSpin_->value());
    preferences_.setDietaryTagIds(checkedIds(dietaryList_));
    account_.setAllergenIds(checkedIds(allergenList_));
    pantry_.setPantryItems(pantryItemsFromTable());

    if (ux_.isSignedIn()) {
        if (ux_.saveCurrentUserInfo(account_, preferences_, pantry_)) {
            preferencesStatusLabel_->setText("Preferences saved to your account.");
            setGlobalStatus("Preferences saved.");
        } else {
            preferencesStatusLabel_->setText("Could not save preferences to disk.");
            setGlobalStatus("Could not save preferences.", true);
        }
    } else {
        preferencesStatusLabel_->setText("Preferences applied for this session. Sign in to save them permanently.");
        setGlobalStatus("Preferences applied for this session.");
    }
}

void MainWindow::generateMealPlan() {
    if (!catalog_.isLoaded()) {
        QMessageBox::critical(this, "Recipe Database", "The recipe catalog is not available.");
        return;
    }

    savePreferences();
    const auto mode = static_cast<MealGenerationMode>(generationModeCombo_->currentData().toInt());
    const MealGenerationResult result = mealGenerator_.generateWeeklyMealPlan(
        mealPlan_, catalog_, account_, preferences_, pantry_, mode
    );

    grocery_.buildFromMealPlan(mealPlan_, catalog_, pantry_);
    refreshMealPlanTable();
    refreshGroceryTable();

    if (!result.generated) {
        mealPlanSummaryLabel_->setText("No compatible meals could be generated with the current selections.");
        setGlobalStatus("Meal plan generation failed. Try removing a restriction or increasing the budget.", true);
        return;
    }

    const QString status = QString("Generated %1 of 21 meals. Estimated grocery total: %2. %3")
            .arg(result.mealsGenerated)
            .arg(money(grocery_.calculateTotal()))
            .arg(grocery_.isWithinBudget(preferences_.getBudget()) ? "Within budget." : "Over budget.");
    mealPlanSummaryLabel_->setText(status);
    setGlobalStatus(status, !result.complete);
}

void MainWindow::refreshMealPlanTable() {
    for (std::size_t day = 0; day < MealPlan::kDaysInWeek; ++day) {
        mealPlanTable_->setItem(static_cast<int>(day), 0, readOnlyItem(QString::fromStdString(MealPlan::dayName(day))));
        const MealType types[3] = {MealType::Breakfast, MealType::Lunch, MealType::Dinner};
        for (int column = 0; column < 3; ++column) {
            const MealEntry* entry = mealPlan_.getMeal(day, types[column]);
            QString title = "Not selected";
            if (entry && entry->recipeId) {
                const auto recipe = catalog_.getRecipeById(*entry->recipeId);
                if (recipe) {
                    title = QString::fromStdString(recipe->title);
                }
            }
            mealPlanTable_->setItem(static_cast<int>(day), column + 1, readOnlyItem(title));
        }
    }

    if (mealPlan_.countMeals() == 0) {
        mealPlanSummaryLabel_->setText("No meal plan has been generated yet.");
    } else {
        mealPlanSummaryLabel_->setText(
            QString("%1 of 21 meal slots filled. Grocery estimate: %2.")
            .arg(mealPlan_.countMeals())
            .arg(money(grocery_.calculateTotal()))
        );
    }
}

void MainWindow::refreshGroceryTable() {
    const auto& items = grocery_.getItems();
    groceryTable_->setRowCount(static_cast<int>(items.size()));
    for (int row = 0; row < static_cast<int>(items.size()); ++row) {
        const GroceryItem& item = items[static_cast<std::size_t>(row)];
        const auto ingredient = catalog_.getIngredientById(item.ingredientId);
        groceryTable_->setItem(row, 0, readOnlyItem(ingredient
                                                        ? QString::fromStdString(ingredient->name)
                                                        : QString("Ingredient #%1").arg(item.ingredientId)));
        groceryTable_->setItem(
            row, 1, readOnlyItem(item.requiredGrams ? QString::number(*item.requiredGrams, 'f', 1) : "Unknown"));
        groceryTable_->setItem(row, 2, readOnlyItem(QString::number(item.purchaseUnits)));
        groceryTable_->setItem(
            row, 3, readOnlyItem(ingredient ? QString::fromStdString(ingredient->purchaseUnitLabel) : "unit"));
        groceryTable_->setItem(row, 4, readOnlyItem(money(item.estimatedCost)));
    }

    const double total = grocery_.calculateTotal();
    const double budget = preferences_.getBudget();
    if (items.empty()) {
        grocerySummaryLabel_->setText("The grocery list is empty. Generate a meal plan first.");
    } else if (grocery_.isWithinBudget(budget)) {
        grocerySummaryLabel_->setText(
            QString("Estimated total: %1 | Budget: %2 | Within budget by %3")
            .arg(money(total), money(budget), money(budget - total))
        );
    } else {
        grocerySummaryLabel_->setText(
            QString("Estimated total: %1 | Budget: %2 | Over budget by %3")
            .arg(money(total), money(budget), money(total - budget))
        );
    }
}

void MainWindow::searchRecipes() {
    RecipeFilter filter;
    const QString query = recipeSearchEdit_->text().trimmed();
    if (!query.isEmpty()) {
        filter.titleContains = query.toStdString();
    }
    currentRecipeResults_ = catalog_.searchRecipes(filter);
    recipeResultsList_->clear();
    for (const Recipe& recipe: currentRecipeResults_) {
        auto* item = new QListWidgetItem(QString::fromStdString(recipe.title));
        item->setData(Qt::UserRole, recipe.recipeId);
        recipeResultsList_->addItem(item);
    }
    recipeDetailsText_->clear();
    if (!currentRecipeResults_.empty()) {
        recipeResultsList_->setCurrentRow(0);
    } else {
        recipeDetailsText_->setPlainText("No matching recipes found.");
    }
}

void MainWindow::displaySelectedRecipe() {
    const int row = recipeResultsList_->currentRow();
    if (row < 0 || row >= static_cast<int>(currentRecipeResults_.size())) {
        recipeDetailsText_->clear();
        return;
    }
    recipeDetailsText_->setPlainText(recipeDetails(currentRecipeResults_[static_cast<std::size_t>(row)]));
}

QString MainWindow::recipeDetails(const Recipe& recipe) const {
    QString text;
    text += QString::fromStdString(recipe.title) + "\n";
    text += QString("Meal type: %1\n").arg(QString::fromStdString(recipe.mealType));
    text += QString("Difficulty: %1\n").arg(QString::fromStdString(recipe.difficulty));
    text += QString("Servings: %1\n").arg(optionalNumber(recipe.servings));
    text += QString("Prep time: %1\n").arg(optionalNumber(recipe.prepMinutes, " minutes"));
    text += QString("Cook time: %1\n").arg(optionalNumber(recipe.cookMinutes, " minutes"));
    text += QString("Primary equipment: %1\n").arg(
        QString::fromStdString(recipe.primaryEquipment.empty() ? "Not provided" : recipe.primaryEquipment));
    if (!recipe.selectionNotes.empty()) {
        text += "Notes: " + QString::fromStdString(recipe.selectionNotes) + "\n";
    }

    text += "\nIngredients\n";
    for (const RecipeIngredient& ingredient: catalog_.getRecipeIngredients(recipe.recipeId)) {
        const std::string line = ingredient.sourceIngredientText.empty()
                                     ? std::to_string(ingredient.quantity) + " " + ingredient.unit + " " + ingredient.
                                       ingredientName
                                     : ingredient.sourceIngredientText;
        text += "• " + QString::fromStdString(line) + "\n";
    }

    const auto seasoners = catalog_.getRecipeSeasoners(recipe.recipeId);
    if (!seasoners.empty()) {
        text += "\nSeasonings\n";
        for (const std::string& seasoner: seasoners) {
            text += "• " + QString::fromStdString(seasoner) + "\n";
        }
    }

    const auto instructions = catalog_.getPreparationInstructions(recipe.recipeId);
    text += "\nPreparation Instructions\n";
    text += instructions ? QString::fromStdString(*instructions) : "No instructions available.";

    double estimatedCost = 0.0;
    bool hasCost = false;
    for (const RecipeCostItem& item: catalog_.getRecipeCostItems({recipe.recipeId})) {
        if (item.requiredGrams) {
            estimatedCost += (*item.requiredGrams / 100.0) * item.pricePer100Grams;
            hasCost = true;
        }
    }
    text += "\n\nEstimated ingredient cost: ";
    text += hasCost ? money(estimatedCost) : "Not available";

    if (!recipe.sourceName.empty()) {
        text += "\nSource: " + QString::fromStdString(recipe.sourceName);
    }
    if (!recipe.sourceUrl.empty()) {
        text += "\nSource URL: " + QString::fromStdString(recipe.sourceUrl);
    }
    return text;
}

std::vector<int> MainWindow::checkedIds(const QListWidget* list) const {
    std::vector<int> ids;
    for (int index = 0; index < list->count(); ++index) {
        const QListWidgetItem* item = list->item(index);
        if (item->checkState() == Qt::Checked) {
            ids.push_back(item->data(Qt::UserRole).toInt());
        }
    }
    return ids;
}

void MainWindow::setCheckedIds(QListWidget* list, const std::vector<int>& ids) {
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem* item = list->item(index);
        const int id = item->data(Qt::UserRole).toInt();
        item->setCheckState(std::find(ids.begin(), ids.end(), id) != ids.end() ? Qt::Checked : Qt::Unchecked);
    }
}

std::vector<PantryItem> MainWindow::pantryItemsFromTable() const {
    std::vector<PantryItem> items;
    for (int row = 0; row < pantryTable_->rowCount(); ++row) {
        const auto* check = qobject_cast<QCheckBox *>(pantryTable_->cellWidget(row, 0));
        const auto* grams = qobject_cast<QDoubleSpinBox *>(pantryTable_->cellWidget(row, 2));
        if (!check || !check->isChecked()) {
            continue;
        }
        const int ingredientId = check->property("ingredientId").toInt();
        const std::optional<double> availableGrams = grams && grams->value() > 0.0
                                                         ? std::optional<double>(grams->value())
                                                         : std::nullopt;
        items.push_back({ingredientId, availableGrams});
    }
    return items;
}

void MainWindow::setPantryTableItems(const std::vector<PantryItem>& items) {
    std::unordered_map<int, std::optional<double> > byId;
    for (const PantryItem& item: items) {
        byId[item.ingredientId] = item.availableGrams;
    }

    for (int row = 0; row < pantryTable_->rowCount(); ++row) {
        auto* check = qobject_cast<QCheckBox *>(pantryTable_->cellWidget(row, 0));
        auto* grams = qobject_cast<QDoubleSpinBox *>(pantryTable_->cellWidget(row, 2));
        if (!check || !grams) {
            continue;
        }
        const int ingredientId = check->property("ingredientId").toInt();
        const auto found = byId.find(ingredientId);
        const bool selected = found != byId.end();
        check->setChecked(selected);
        grams->setEnabled(selected);
        grams->setValue(selected && found->second ? *found->second : 0.0);
    }
}

bool MainWindow::requireSignIn(const QString& action) {
    if (ux_.isSignedIn()) {
        return true;
    }
    QMessageBox::information(this, "Sign In Required", "Please sign in before " + action + ".");
    showPage(AccountPageIndex);
    return false;
}
