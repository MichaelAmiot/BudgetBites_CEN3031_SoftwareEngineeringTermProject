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
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
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
// Formats a plain number as a dollar amount, always with two decimal places.
QString money(double value) {
    return QString("$%1").arg(value, 0, 'f', 2);
}

// Some Qt styles/platforms won't render a custom QSS arrow image on a spin box's
//  up/down buttons, even though the buttons themselves stay fully clickable. This
//  overlays plain text labels (rendered the normal Qt way, no image loading involved)
//  on top of the button area so the direction is always visibly clear, and keeps
//  them positioned correctly whenever the spin box is resized.
class SpinBoxArrowOverlay : public QObject {
public:
    SpinBoxArrowOverlay(QDoubleSpinBox* spinBox, QLabel* upLabel, QLabel* downLabel)
        : QObject(spinBox), spinBox_(spinBox), upLabel_(upLabel), downLabel_(downLabel) {
        reposition();
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == spinBox_ && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
            reposition();
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void reposition() const {
        constexpr int buttonWidth = 24;
        const int height = spinBox_->height();
        const int halfHeight = height / 2;
        const int x = spinBox_->width() - buttonWidth;
        upLabel_->setGeometry(x, 0, buttonWidth, halfHeight);
        downLabel_->setGeometry(x, halfHeight, buttonWidth, height - halfHeight);
    }

    QDoubleSpinBox* spinBox_;
    QLabel* upLabel_;
    QLabel* downLabel_;
};

// Attaches visible ▲/▼ indicator labels on top of a spin box's native up/down
//  buttons. The labels are click-through, so the spin box's own buttons keep
//  handling the actual increment/decrement.
void addSpinBoxArrowIndicators(QDoubleSpinBox* spinBox) {
    auto* upLabel = new QLabel(QString::fromUtf8("\xE2\x96\xB2"), spinBox);
    auto* downLabel = new QLabel(QString::fromUtf8("\xE2\x96\xBC"), spinBox);
    for (QLabel* label : {upLabel, downLabel}) {
        label->setAlignment(Qt::AlignCenter);
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        label->setStyleSheet("color: #f5f7fa; background: transparent; font-size: 8px;");
        label->show();
    }
    spinBox->installEventFilter(new SpinBoxArrowOverlay(spinBox, upLabel, downLabel));
}

// Draws a small eye icon for the password show/hide toggle. Drawn by hand
//  with QPainter instead of loading an image file, since that has proven far
//  more reliable than image resources anywhere else in this project.
//  visible = true draws an open eye with a pupil (password is currently shown)
//  visible = false draws the same eye with a line through it (password is currently hidden).
QIcon makePasswordEyeIcon(bool visible) {
    constexpr int size = 20;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const QColor color(174, 180, 183); // matches the app's muted text color
    QPen outlinePen(color);
    outlinePen.setWidthF(1.6);
    painter.setPen(outlinePen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath eyePath;
    eyePath.moveTo(2, 10);
    eyePath.cubicTo(6, 3, 14, 3, 18, 10);
    eyePath.cubicTo(14, 17, 6, 17, 2, 10);
    painter.drawPath(eyePath);

    if (visible) {
        painter.setBrush(color);
        painter.drawEllipse(QPointF(10, 10), 3.2, 3.2);
    } else {
        painter.drawLine(QPointF(3, 17), QPointF(17, 3));
    }

    painter.end();
    return QIcon(pixmap);
}

// A small gray label used for subtitles and helper text under a page title.
QLabel* makeMutedLabel(const QString& text) {
    auto* label = new QLabel(text);
    label->setObjectName("mutedLabel");
    label->setWordWrap(true);
    return label;
}

// Wraps plain text in a table item that the user cannot type into. Used for
//  every table cell that just displays information instead of collecting it.
QTableWidgetItem* readOnlyItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

// Turns an optional catalog id into text for display, with a fallback
//  message when nothing was set.
QString optionalNumber(const std::optional<int>& value, const QString& suffix = {}) {
    return value ? QString::number(*value) + suffix : QString("Not provided");
}
} // namespace

// Sets up every page, connects every button, and loads the saved account
//  list from disk so people can sign back in without registering again.
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    projectRoot_(BUDGETBITES_SOURCE_DIR),
    usersFile_(projectRoot_ / UserRepository::defaultStoragePath()),
    ux_(projectRoot_ / "data" / "local"),
    catalog_(projectRoot_ / "data" / "seed") {
    ui->setupUi(this);

    // Each screen is its own Qt Designer form. MainWindow only hosts and
    //  coordinates those pages while the existing backend classes remain here.
    dashboardPageWidget_ = new DashboardPage(this);
    accountPageWidget_ = new AccountPage(this);
    preferencesPageWidget_ = new PreferencesPage(this);
    mealPlanPageWidget_ = new MealPlanPage(this);
    recipePageWidget_ = new RecipePage(this);
    groceryPageWidget_ = new GroceryPage(this);

    ui->pages->addWidget(dashboardPageWidget_);
    ui->pages->addWidget(accountPageWidget_);

    // The Preferences page contains more vertical content than the other pages.
    // Wrap it in a scroll area so Qt does not compress or overlap its widgets.
    auto* preferencesScrollArea = new QScrollArea(this);
    preferencesScrollArea->setObjectName("preferencesScrollArea");
    preferencesScrollArea->setWidgetResizable(true);
    preferencesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    preferencesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    preferencesScrollArea->setFrameShape(QFrame::NoFrame);

    preferencesPageWidget_->setMinimumHeight(820);
    preferencesScrollArea->setWidget(preferencesPageWidget_);

    ui->pages->addWidget(preferencesScrollArea);
    ui->pages->addWidget(mealPlanPageWidget_);
    ui->pages->addWidget(recipePageWidget_);
    ui->pages->addWidget(groceryPageWidget_);

    pages_ = ui->pages;
    signedInLabel_ = ui->signedInLabel;
    headerAvatarLabel_ = ui->headerAvatarLabel;
    globalStatusLabel_ = ui->globalStatusLabel;

    auto* dashboardUi = dashboardPageWidget_->ui();
    auto* accountUi = accountPageWidget_->ui();
    auto* preferencesUi = preferencesPageWidget_->ui();
    auto* mealPlanUi = mealPlanPageWidget_->ui();
    auto* recipeUi = recipePageWidget_->ui();
    auto* groceryUi = groceryPageWidget_->ui();

    usernameEdit_ = accountUi->usernameEdit;
    passwordEdit_ = accountUi->passwordEdit;
    passwordEdit_->setEchoMode(QLineEdit::Password);
    // Adds a clickable eye icon inside the password field itself, using
    //  Qt's built in support for actions on a line edit. Clicking it flips
    //  between hiding the password and showing it in plain text, and swaps
    //  the icon to match whichever state it just switched to.
    auto* passwordVisibilityAction = passwordEdit_->addAction(
        makePasswordEyeIcon(false), QLineEdit::TrailingPosition);
    passwordVisibilityAction->setToolTip("Show password");
    connect(passwordVisibilityAction, &QAction::triggered, passwordEdit_,
            [passwordEdit = passwordEdit_, passwordVisibilityAction] {
                const bool currentlyVisible = passwordEdit->echoMode() == QLineEdit::Normal;
                passwordEdit->setEchoMode(currentlyVisible ? QLineEdit::Password : QLineEdit::Normal);
                passwordVisibilityAction->setIcon(makePasswordEyeIcon(!currentlyVisible));
                passwordVisibilityAction->setToolTip(currentlyVisible ? "Show password" : "Hide password");
            });
    accountStatusLabel_ = accountUi->accountStatusLabel;
    signOutButton_ = accountUi->signOutButton;
    // Photo preview and its two buttons, filled in and enabled/disabled
    //  later by refreshProfileImagePreview and updateSignedInState.
    profileImagePreview_ = accountUi->profileImagePreview;
    uploadPhotoButton_ = accountUi->uploadPhotoButton;
    removePhotoButton_ = accountUi->removePhotoButton;

    budgetSpin_ = preferencesUi->budgetSpin;
    // Draws visible up/down arrows on top of the spin box's native buttons.
    // See the comment on addSpinBoxArrowIndicators above for why.
    addSpinBoxArrowIndicators(budgetSpin_);
    dietaryList_ = preferencesUi->dietaryList_4;
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

    // Column sizing for the three tables. Stretch columns fill leftover
    //  space, ResizeToContents columns shrink to fit whatever is in them.
    pantryTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    pantryTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    pantryTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    pantryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Each option stores its MealGenerationMode value alongside the label,
    //  so generateMealPlan can just read back whichever one is selected.
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

    // Each dashboard tile just jumps to its matching page.
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

    // Every page's back button does the same thing, so one loop wires them all.
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

    // Account page buttons and fields.
    connect(accountUi->registerButton, &QPushButton::clicked, this, &MainWindow::registerUser);
    connect(accountUi->signInButton, &QPushButton::clicked, this, &MainWindow::signIn);
    connect(accountUi->signOutButton, &QPushButton::clicked, this, &MainWindow::signOut);
    connect(passwordEdit_, &QLineEdit::returnPressed, this, &MainWindow::signIn);
    connect(usernameEdit_, &QLineEdit::editingFinished, this, &MainWindow::checkUsernameAvailability);
    connect(uploadPhotoButton_, &QPushButton::clicked, this, &MainWindow::uploadProfilePhoto);
    connect(removePhotoButton_, &QPushButton::clicked, this, &MainWindow::removeProfilePhoto);

    // Preferences page.
    connect(preferencesUi->savePreferencesButton, &QPushButton::clicked, this, &MainWindow::savePreferences);
    connect(preferencesUi->reloadPreferencesButton, &QPushButton::clicked, this,
            &MainWindow::loadPreferenceControlsFromState);
    connect(preferencesUi->clearPantryButton, &QPushButton::clicked, this,
            &MainWindow::clearPantryTable);

    // Meal plan page.
    connect(mealPlanUi->generateMealPlanButton, &QPushButton::clicked, this, &MainWindow::generateMealPlan);
    connect(mealPlanUi->clearMealPlanButton, &QPushButton::clicked, this, &MainWindow::clearMealPlan);
    connect(mealPlanUi->openGroceryButton, &QPushButton::clicked, this, [this] {
        refreshGroceryTable();
        showPage(GroceryPageIndex);
    });

    // Recipe page. Selecting a different row in the results list refreshes
    //  the details panel on the right.
    connect(recipeUi->recipeSearchButton, &QPushButton::clicked, this, &MainWindow::searchRecipes);
    connect(recipeSearchEdit_, &QLineEdit::returnPressed, this, &MainWindow::searchRecipes);
    connect(recipeResultsList_, &QListWidget::currentRowChanged, this, [this](int) {
        displaySelectedRecipe();
    });

    // Grocery page.
    connect(groceryUi->rebuildGroceryButton, &QPushButton::clicked, this, [this] {
        grocery_.buildFromMealPlan(mealPlan_, catalog_, pantry_);
        refreshGroceryTable();
    });
    connect(groceryUi->openMealPlanButton, &QPushButton::clicked, this, [this] {
        showPage(MealPlanPageIndex);
    });

    // Everything above just wires the UI together. This is where the app
    //  actually loads whatever accounts were saved from a previous run and shows the first page.
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


// Runs right before the window actually closes, so the signed in user's
//  budget, preferences, and pantry get saved even if they just click the X
//  button instead of using an explicit save option.
void MainWindow::closeEvent(QCloseEvent* event) {
    ux_.saveToFile(usersFile_.string());
    if (ux_.isSignedIn()) {
        ux_.saveCurrentUserInfo(account_, preferences_, pantry_);
    }
    QMainWindow::closeEvent(event);
}

// One big stylesheet for the whole app: dark background, accent colors,
//  and shared rules for buttons, tables, and form fields. Anything not
//  covered here falls back to whatever a page's own .ui file sets.
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
        QDoubleSpinBox {
            padding-right: 26px;
        }
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
            subcontrol-origin: border;
            width: 24px;
            border-left: 1px solid #3a414c;
            background: #2a2f38;
        }
        QDoubleSpinBox::up-button {
            subcontrol-position: top right;
            border-top-right-radius: 6px;
            border-bottom: 1px solid #3a414c;
        }
        QDoubleSpinBox::down-button {
            subcontrol-position: bottom right;
            border-bottom-right-radius: 6px;
        }
        QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
            background: #2f7df6;
        }
        QDoubleSpinBox::up-button:pressed, QDoubleSpinBox::down-button:pressed {
            background: #246bd8;
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

// Switches to the given page, refreshing that page's data first if it
//  depends on something that could have changed since the last visit.
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

// Updates the thin status bar at the bottom of the window. Turns the text
//  red when error is true, otherwise uses the normal muted gray.
void MainWindow::setGlobalStatus(const QString& message, bool error) {
    globalStatusLabel_->setText(message);
    globalStatusLabel_->setStyleSheet(error ? "color: #ff8f8f;" : "color: #aeb6c2;");
}

// Refreshes anything in the header or account page that depends on whether
//  someone is currently signed in. Called after signing in, signing out, and
//  once at startup to set the initial state.
void MainWindow::updateSignedInState() {
    if (ux_.isSignedIn()) {
        const QString username = QString::fromStdString(*ux_.currentUser());
        signedInLabel_->setText("Signed in as " + username);
        accountStatusLabel_->setText("Signed in as " + username + ". Your saved information is loaded.");
        signOutButton_->setEnabled(true);
        uploadPhotoButton_->setEnabled(true);
    } else {
        signedInLabel_->setText("Not signed in");
        accountStatusLabel_->setText(
            "Not signed in. You may browse recipes, but sign in to save personalized information.");
        signOutButton_->setEnabled(false);
        uploadPhotoButton_->setEnabled(false);
        removePhotoButton_->setEnabled(false);
    }
    refreshProfileImagePreview();
}

// Warns the user right away if the username they just typed is taken,
//  before they even get to choosing a password.
void MainWindow::checkUsernameAvailability() {
    const std::string username = usernameEdit_->text().trimmed().toStdString();
    if (username.empty()) {
        return;
    }
    if (ux_.isUsernameTaken(username)) {
        accountStatusLabel_->setText(
            "That username is already taken. Please choose a different username.");
    } else {
        accountStatusLabel_->setText(
            "Username available. Choose a password (8+ chars, upper, lower, digit, special character).");
    }
}

// Registers a brand new account from whatever is in the username and
//  password fields, checking the username a second time in case something
//  registered it in the moment between typing and clicking this button.
void MainWindow::registerUser() {
    const std::string username = usernameEdit_->text().trimmed().toStdString();
    const std::string password = passwordEdit_->text().toStdString();
    if (ux_.isUsernameTaken(username)) {
        accountStatusLabel_->setText(
            "That username is already taken. Please choose a different username.");
        return;
    }
    if (!ux_.registerUser(username, password)) {
        accountStatusLabel_->setText(
            "Registration failed. Password must be 8+ chars with upper, lower, digit, and special character.");
        return;
    }
    ux_.saveToFile(usersFile_.string());
    accountStatusLabel_->setText("Registration succeeded. You can now sign in.");
    passwordEdit_->clear();
}

// Signs in with whatever is in the username and password fields, then
//  resets every business object and reloads that user's saved settings so
//  nothing from a previous session leaks through.
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

// Saves whatever the user currently has set before signing them out, then
//  resets every business object back to a clean, signed out state.
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

// Opens a file picker for an image, hands the chosen path to the existing
//  upload logic, then refreshes both the account page thumbnail and the
//  small header avatar so the new photo shows up right away.
void MainWindow::uploadProfilePhoto() {
    if (!requireSignIn("uploading a profile photo")) {
        return;
    }

    const QString path = QFileDialog::getOpenFileName(
        this,
        "Choose a profile photo",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.gif)"
    );
    if (path.isEmpty()) {
        return;
    }

    const std::string username = *ux_.currentUser();
    if (!ux_.uploadProfileImage(username, path.toStdString())) {
        accountStatusLabel_->setText(
            "Could not upload that photo. Only png, jpg, jpeg, bmp, and gif files are supported.");
        return;
    }

    ux_.saveToFile(usersFile_.string());
    refreshProfileImagePreview();
    accountStatusLabel_->setText("Profile photo updated.");
}

// Deletes the current profile photo, if there is one, and resets the
//  preview labels back to the placeholder state.
void MainWindow::removeProfilePhoto() {
    if (!requireSignIn("removing a profile photo")) {
        return;
    }

    const std::string username = *ux_.currentUser();
    if (!ux_.removeProfileImage(username)) {
        accountStatusLabel_->setText("Could not remove the profile photo.");
        return;
    }

    ux_.saveToFile(usersFile_.string());
    refreshProfileImagePreview();
    accountStatusLabel_->setText("Profile photo removed.");
}

void MainWindow::refreshProfileImagePreview() {
    // Applies the same profile photo (or the same placeholder) to any label
    //  that should show the user's picture, scaled to whatever size that particular label is.
    const auto applyToLabel = [](QLabel* label, const QPixmap& pixmap, const QString& placeholderText) {
        if (!pixmap.isNull()) {
            label->setPixmap(pixmap.scaled(
                label->size(),
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
            ));
        } else {
            label->setPixmap(QPixmap());
            label->setText(placeholderText);
        }
    };

    QPixmap pixmap;
    if (ux_.isSignedIn()) {
        const auto imagePath = ux_.getProfileImagePath(*ux_.currentUser());
        if (imagePath) {
            pixmap = QPixmap(QString::fromStdString(*imagePath));
        }
    }

    applyToLabel(profileImagePreview_, pixmap, "No Photo");
    applyToLabel(headerAvatarLabel_, pixmap, "");
    removePhotoButton_->setEnabled(ux_.isSignedIn() && !pixmap.isNull());
}

// Fills the dietary, allergen, and pantry lists from the recipe catalog.
// Runs once at startup, since the catalog itself never changes while the
//  app is running. Each checkbox item remembers its catalog id in
//  Qt::UserRole so checkedIds can read it back later.
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
        addSpinBoxArrowIndicators(grams);
        pantryTable_->setCellWidget(row, 2, grams);
        connect(check, &QCheckBox::toggled, grams, &QWidget::setEnabled);
    }
}

// Pushes whatever is currently in preferences_, account_, and pantry_ onto
//  the on screen controls. Called when the preferences page is opened and right after signing in or out.
void MainWindow::loadPreferenceControlsFromState() {
    budgetSpin_->setValue(preferences_.getBudget());
    setCheckedIds(dietaryList_, preferences_.getDietaryTagIds());
    setCheckedIds(allergenList_, account_.getAllergenIds());
    setPantryTableItems(pantry_.getPantryItems());
}

// Unchecks every row in the pantry table and clears its gram amount back
//  to disabled/zero. Leaves the budget, dietary, and allergen controls
//  untouched. Nothing is written to disk here — the person still has to
//  click Save Preferences to make the clear stick, and Reload Saved
//  Values will bring back whatever pantry state was last saved if they
//  change their mind first.
void MainWindow::clearPantryTable() {
    const auto choice = QMessageBox::question(
        this,
        "Clear Pantry",
        "This unchecks every ingredient in your pantry inventory and clears its available grams. "
        "Click Save Preferences afterward to make this permanent. Continue?",
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel
        );
    if (choice != QMessageBox::Yes) {
        return;
    }

    setPantryTableItems({});

    preferencesStatusLabel_->setText("Pantry cleared on screen. Click Save Preferences to make this permanent.");
    setGlobalStatus("Pantry inventory cleared on screen (not yet saved).");
}

// Reads whatever is currently on screen back into preferences_, account_,
//  and pantry_, then saves it to disk if someone is signed in. If nobody
//  is signed in the values still apply for the rest of this session, they
//  just will not be there next time the app opens.
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

// Saves the current preferences first, so the generator always works from
//  the budget and restrictions actually shown on screen, then builds a
//  weekly meal plan and the grocery list that goes with it.
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

    const bool withinBudget = grocery_.isWithinBudget(preferences_.getBudget());
    const QString status = QString("Generated %1 of 21 meals. Estimated grocery total: %2. %3")
                               .arg(result.mealsGenerated)
                               .arg(money(grocery_.calculateTotal()))
                               .arg(withinBudget ? "Within budget." : "Over budget.");
    mealPlanSummaryLabel_->setText(status);
    mealPlanSummaryLabel_->setStyleSheet(withinBudget ? "" : "color: #ff8f8f;");
    setGlobalStatus(status, !result.complete);
}

// Clears the generated weekly meal plan and its grocery list back to
//  empty. Preferences (budget, dietary tags, allergens, pantry) are left
//  untouched, so pressing Generate Weekly Plan again immediately after
//  still uses the same restrictions, just with a fresh, empty starting
//  point instead of building on top of the previous plan.
void MainWindow::clearMealPlan() {
    const auto choice = QMessageBox::question(
        this,
        "Clear Meal Plan",
        "This clears your current weekly meal plan and grocery list. Continue?",
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel
        );
    if (choice != QMessageBox::Yes) {
        return;
    }

    mealPlan_ = MealPlan{};
    grocery_ = Grocery{};
    refreshMealPlanTable();
    refreshGroceryTable();

    mealPlanSummaryLabel_->setText("No meal plan yet. Choose a generation mode and create your weekly plan.");
    mealPlanSummaryLabel_->setStyleSheet("");
    setGlobalStatus("Meal plan cleared.");
}

// Rebuilds the seven day meal plan table from whatever is currently stored
//  in mealPlan_, and updates the summary line above it.
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

// Rebuilds the grocery list table from grocery_ and shows whether the
//  current total is within budget, coloring the summary line red if not.
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
        grocerySummaryLabel_->setStyleSheet("");
    } else if (grocery_.isWithinBudget(budget)) {
        grocerySummaryLabel_->setText(
            QString("Estimated total: %1 | Budget: %2 | Within budget by %3")
                .arg(money(total), money(budget), money(budget - total))
            );
        grocerySummaryLabel_->setStyleSheet("");
    } else {
        grocerySummaryLabel_->setText(
            QString("Estimated total: %1 | Budget: %2 | Over budget by %3")
                .arg(money(total), money(budget), money(total - budget))
            );
        grocerySummaryLabel_->setStyleSheet("color: #ff8f8f;");
    }
}

// Filters the catalog by whatever text is in the search box and fills the
//  results list with matches. Selects the first result automatically so the
//  details panel is not left blank after a search.
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

// Shows the full details for whichever recipe is currently selected in the
//  results list, or clears the panel if nothing valid is selected.
void MainWindow::displaySelectedRecipe() {
    const int row = recipeResultsList_->currentRow();
    if (row < 0 || row >= static_cast<int>(currentRecipeResults_.size())) {
        recipeDetailsText_->clear();
        return;
    }
    recipeDetailsText_->setPlainText(recipeDetails(currentRecipeResults_[static_cast<std::size_t>(row)]));
}

// Builds the plain text block shown in the recipe details panel: basic
//  info, ingredients, seasonings, instructions, and an estimated cost.
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

// Collects the catalog ids of every checked item in a list, used for both
//  the dietary preferences list and the allergen list.
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

// The opposite of checkedIds: ticks whichever items in the list have an id
//  that appears in the given set, and unchecks everything else.
void MainWindow::setCheckedIds(QListWidget* list, const std::vector<int>& ids) {
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem* item = list->item(index);
        const int id = item->data(Qt::UserRole).toInt();
        item->setCheckState(std::find(ids.begin(), ids.end(), id) != ids.end() ? Qt::Checked : Qt::Unchecked);
    }
}

// Reads the pantry table back into a list of PantryItem, skipping any row
//  whose checkbox is not checked. A gram amount of zero or less is treated
//  as unknown rather than an actual amount on hand.
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

// The opposite of pantryItemsFromTable: checks the box and fills in the
//  gram amount for every ingredient that appears in the given list, and
//  leaves every other row unchecked and disabled.
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

// Checks whether someone is signed in before letting an action continue.
// If nobody is, shows a message explaining why and jumps to the account
//  page so they can sign in right there.
bool MainWindow::requireSignIn(const QString& action) {
    if (ux_.isSignedIn()) {
        return true;
    }
    QMessageBox::information(this, "Sign In Required", "Please sign in before " + action + ".");
    showPage(AccountPageIndex);
    return false;
}
