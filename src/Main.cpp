#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Grocery.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/MainHelper.h"
#include "BudgetBitesLib/MealGenerator.h"
#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/Preferences.h"
#include "BudgetBitesLib/RecipeDataBase.h"
#include "BudgetBitesLib/UserRepository.h"
#include "BudgetBitesLib/UX.h"

#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace {

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::optional<int> readInteger(const std::string& prompt) {
    const std::string input = readLine(prompt);
    try {
        std::size_t used = 0;
        const int value = std::stoi(input, &used);
        return used == input.size() ? std::optional<int>(value) : std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> readNumber(const std::string& prompt) {
    const std::string input = readLine(prompt);
    try {
        std::size_t used = 0;
        const double value = std::stod(input, &used);
        return used == input.size() ? std::optional<double>(value) : std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::string readPassword(const std::string& prompt) {
    std::cout << prompt;
    std::string password;

#ifdef _WIN32
    char character;
    while ((character = static_cast<char>(_getch())) != '\r' && character != '\n') {
        if (character == '\b') {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        } else {
            password.push_back(character);
            std::cout << '*';
        }
    }
#else
    termios oldSettings{};
    if (tcgetattr(STDIN_FILENO, &oldSettings) == 0) {
        termios newSettings = oldSettings;
        newSettings.c_lflag &= ~static_cast<tcflag_t>(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);
        std::getline(std::cin, password);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
    } else {
        std::getline(std::cin, password);
    }
#endif

    std::cout << '\n';
    return password;
}

// Account and Preferences use formatted integer input. Clear the trailing
// newline before Main returns to getline-based menu input.
void finishFormattedInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printSignedOutMenu() {
    std::cout << "\n=== BudgetBites ===\n"
              << "1. Register new user\n"
              << "2. Sign in\n"
              << "3. Exit\n";
}

void printSignedInMenu(const std::string& username) {
    std::cout << "\n=== BudgetBites - " << username << " ===\n"
              << "1. View profile and saved settings\n"
              << "2. Set weekly budget\n"
              << "3. Select dietary preferences\n"
              << "4. Select food allergies\n"
              << "5. Set pantry ingredients\n"
              << "6. Upload profile image\n"
              << "7. View profile image path\n"
              << "8. Generate weekly meal plan\n"
              << "9. View weekly meal plan\n"
              << "10. View recipe details and instructions\n"
              << "11. Replace one meal\n"
              << "12. View grocery list and budget result\n"
              << "13. Sign out\n"
              << "14. Exit\n";
}

std::optional<std::size_t> readDayIndex() {
    const auto day = readInteger("Day (1 = Monday, 7 = Sunday): ");
    if (!day || *day < 1 || *day > static_cast<int>(MealPlan::kDaysInWeek)) {
        std::cout << "Please enter a day from 1 to 7.\n";
        return std::nullopt;
    }
    return static_cast<std::size_t>(*day - 1);
}

std::optional<MealType> readMealType() {
    std::cout << "1. Breakfast\n2. Lunch\n3. Dinner\n";
    const auto choice = readInteger("Meal position: ");
    if (!choice) {
        return std::nullopt;
    }
    if (*choice == 1) {
        return MealType::Breakfast;
    }
    if (*choice == 2) {
        return MealType::Lunch;
    }
    if (*choice == 3) {
        return MealType::Dinner;
    }
    std::cout << "Please choose breakfast, lunch, or dinner.\n";
    return std::nullopt;
}

std::optional<MealGenerationMode> readMealGenerationMode() {
    std::cout << "\nChoose meal-plan mode:\n"
              << "1. Normal plan\n"
              << "   Generates all 21 meals with more variety. It may exceed your budget.\n"
              << "2. Budget-first plan\n"
              << "   Generates all 21 meals while prioritizing your budget.\n"
              << "   Repeat limits are relaxed when needed, and the result may exceed your budget.\n"
              << "3. Strict-budget plan\n"
              << "   Never exceeds your budget, but may generate fewer than 21 meals.\n";
    const auto choice = readInteger("Mode: ");
    if (!choice) {
        return std::nullopt;
    }
    if (*choice == 1) {
        return MealGenerationMode::Normal;
    }
    if (*choice == 2) {
        return MealGenerationMode::BudgetFirst;
    }
    if (*choice == 3) {
        return MealGenerationMode::StrictBudget;
    }
    return std::nullopt;
}

void clearGeneratedPlan(
    MealPlan& mealPlan,
    Grocery& grocery,
    std::optional<MealGenerationMode>& currentPlanMode
) {
    if (mealPlan.countMeals() == 0) {
        return;
    }
    mealPlan.clearAllMeals();
    grocery = Grocery{};
    currentPlanMode.reset();
    std::cout << "The previous meal plan was cleared because user settings changed.\n";
}

bool saveAccountData(UX& ux, const std::string& accountPath) {
    if (ux.saveToFile(accountPath)) {
        return true;
    }
    std::cout << "Unable to save account data.\n";
    return false;
}

bool saveUserSettings(
    UX& ux,
    const Account& account,
    const Preferences& preferences,
    const Ingredients& ingredients
) {
    if (ux.saveCurrentUserInfo(account, preferences, ingredients)) {
        return true;
    }
    std::cout << "Unable to save the current user's settings.\n";
    return false;
}

void handleRegister(UX& ux, const std::string& accountPath) {
    const std::string username = readLine("Choose a username: ");
    const std::string password = readPassword(
        "Choose a password (8+ chars, upper, lower, digit, special character): "
    );

    if (!ux.registerUser(username, password)) {
        std::cout << "Registration failed. Check the username and password requirements.\n";
        return;
    }
    saveAccountData(ux, accountPath);
    std::cout << "Registered successfully. You can now sign in.\n";
}

bool handleSignIn(
    UX& ux,
    Account& account,
    Preferences& preferences,
    Ingredients& ingredients,
    MealPlan& mealPlan,
    Grocery& grocery
) {
    const std::string username = readLine("Username: ");
    const std::string password = readPassword("Password: ");
    if (!ux.signIn(username, password)) {
        std::cout << "Sign-in failed: incorrect username or password.\n";
        return false;
    }

    MainHelper::resetSessionState(account, preferences, ingredients, mealPlan, grocery);
    if (!ux.loadCurrentUserInfo(account, preferences, ingredients)) {
        ux.signOut();
        std::cout << "Unable to load this user's saved settings.\n";
        return false;
    }

    std::cout << "Signed in as " << *ux.currentUser() << ".\n";
    return true;
}

void displayUserSettings(
    UX& ux,
    const Account& account,
    const Preferences& preferences,
    const Ingredients& ingredients,
    const RecipeDataBase& catalog
) {
    std::cout << "\nUsername: " << *ux.currentUser() << '\n';
    preferences.displayDietaryPreferences(catalog.getDietaryTags());
    account.displayFoodAllergies(catalog.getAllergens());
    ingredients.displayIngredients(catalog.getAllIngredients());
    std::cout << "Weekly budget: $" << preferences.getBudget() << '\n';

    const auto imagePath = ux.getProfileImagePath(*ux.currentUser());
    std::cout << "Profile image: "
              << (imagePath ? *imagePath : "Not uploaded") << '\n';
}

void handleUploadImage(UX& ux, const std::string& accountPath) {
    const std::string path = readLine("Path to image file (.png/.jpg/.jpeg/.bmp/.gif): ");
    if (!ux.uploadProfileImage(*ux.currentUser(), path)) {
        std::cout << "Upload failed. Check the path and image extension.\n";
        return;
    }
    saveAccountData(ux, accountPath);
    std::cout << "Profile image uploaded successfully.\n";
}

void handleGenerateMealPlan(
    MealGenerator& generator,
    MealPlan& mealPlan,
    Grocery& grocery,
    const RecipeDataBase& catalog,
    const Account& account,
    const Preferences& preferences,
    const Ingredients& ingredients,
    std::optional<MealGenerationMode>& currentPlanMode
) {
    const auto mode = readMealGenerationMode();
    if (!mode) {
        std::cout << "Please choose a mode from 1 to 3.\n";
        return;
    }

    const MealGenerationResult result = generator.generateWeeklyMealPlan(
        mealPlan,
        catalog,
        account,
        preferences,
        ingredients,
        *mode
    );
    grocery.buildFromMealPlan(mealPlan, catalog, ingredients);

    if (!result.generated) {
        currentPlanMode.reset();
        if (*mode == MealGenerationMode::StrictBudget) {
            std::cout << "No meals could be planned within this budget.\n";
        } else {
            std::cout << "Unable to generate a complete plan with the current restrictions.\n";
        }
        return;
    }

    currentPlanMode = *mode;
    if (*mode == MealGenerationMode::Normal) {
        std::cout << "Normal weekly meal plan generated.\n";
    } else if (*mode == MealGenerationMode::BudgetFirst) {
        std::cout << "Budget-first weekly meal plan generated.\n";
    } else {
        std::cout << "Strict-budget meal plan generated.\n"
                  << "Meals planned: " << result.mealsGenerated
                  << " / " << MealPlan::kDaysInWeek * 3 << '\n';
    }

    if (!result.complete && *mode != MealGenerationMode::StrictBudget) {
        std::cout << "Unable to generate a complete plan with the current restrictions.\n";
    }
    MainHelper::displayBudgetStatus(grocery, preferences, std::cout);
}

void handleRecipeDetails(const MealPlan& mealPlan, const RecipeDataBase& catalog) {
    if (mealPlan.countMeals() == 0) {
        std::cout << "Generate a meal plan first.\n";
        return;
    }
    const auto dayIndex = readDayIndex();
    if (!dayIndex) {
        return;
    }
    const auto mealType = readMealType();
    if (!mealType ||
        !MainHelper::displayRecipeDetails(mealPlan, *dayIndex, *mealType, catalog, std::cout)) {
        std::cout << "That meal does not contain an available recipe.\n";
    }
}

void handleReplaceMeal(
    MealPlan& mealPlan,
    Grocery& grocery,
    const RecipeDataBase& catalog,
    const Account& account,
    const Preferences& preferences,
    const Ingredients& ingredients,
    const std::optional<MealGenerationMode>& currentPlanMode
) {
    if (mealPlan.countMeals() == 0) {
        std::cout << "Generate a meal plan first.\n";
        return;
    }
    const auto dayIndex = readDayIndex();
    if (!dayIndex) {
        return;
    }
    const auto mealType = readMealType();
    if (!mealType) {
        return;
    }

    const std::vector<Recipe> candidates = MainHelper::compatibleRecipesForMeal(
        *mealType,
        catalog,
        account,
        preferences
    );
    if (candidates.empty()) {
        std::cout << "No compatible replacement recipes are available.\n";
        return;
    }

    std::cout << "\nCompatible recipes (ID: title):\n";
    MainHelper::displayRecipeOptions(candidates, std::cout);
    const auto recipeId = readInteger("Replacement recipe ID: ");
    const MealPlan originalPlan = mealPlan;
    const Grocery originalGrocery = grocery;
    if (!recipeId || !MainHelper::replaceMeal(
            mealPlan,
            grocery,
            *dayIndex,
            *mealType,
            *recipeId,
            catalog,
            account,
            preferences,
            ingredients)) {
        std::cout << "That recipe cannot be used for this meal.\n";
        return;
    }

    if (currentPlanMode == MealGenerationMode::StrictBudget &&
        !grocery.isWithinBudget(preferences.getBudget())) {
        mealPlan = originalPlan;
        grocery = originalGrocery;
        std::cout << "That replacement would exceed the strict budget.\n";
        return;
    }

    std::cout << "Meal replaced and grocery estimate updated.\n";
    MainHelper::displayBudgetStatus(grocery, preferences, std::cout);
}

void closeSession(
    UX& ux,
    Account& account,
    Preferences& preferences,
    Ingredients& ingredients,
    MealPlan& mealPlan,
    Grocery& grocery,
    const std::string& accountPath
) {
    saveUserSettings(ux, account, preferences, ingredients);
    saveAccountData(ux, accountPath);
    ux.signOut();
    MainHelper::resetSessionState(account, preferences, ingredients, mealPlan, grocery);
}

} // namespace

int main() {
    RecipeDataBase catalog;
    if (!catalog.isLoaded()) {
        std::cout << "Unable to load the recipe catalog: " << catalog.lastError() << '\n';
        return 1;
    }

    const std::string accountPath = UserRepository::defaultStoragePath();
    const std::filesystem::path accountDirectory = std::filesystem::path(accountPath).parent_path();
    if (!accountDirectory.empty()) {
        std::filesystem::create_directories(accountDirectory);
    }

    UX ux;
    if (std::filesystem::exists(accountPath) && !ux.loadFromFile(accountPath)) {
        std::cout << "Unable to load existing account data.\n";
        return 1;
    }

    Account account;
    Preferences preferences;
    Ingredients ingredients;
    MealPlan mealPlan;
    Grocery grocery;
    MealGenerator mealGenerator;
    std::optional<MealGenerationMode> currentPlanMode;
    bool running = true;

    while (running) {
        if (!ux.isSignedIn()) {
            printSignedOutMenu();
            const auto choice = readInteger("Choose an option: ");
            if (!choice) {
                std::cout << "Please enter a number from the menu.\n";
                continue;
            }

            switch (*choice) {
                case 1:
                    handleRegister(ux, accountPath);
                    break;
                case 2:
                    if (handleSignIn(ux, account, preferences, ingredients, mealPlan, grocery)) {
                        currentPlanMode.reset();
                    }
                    break;
                case 3:
                    saveAccountData(ux, accountPath);
                    running = false;
                    break;
                default:
                    std::cout << "Please choose an option from 1 to 3.\n";
            }
            continue;
        }

        printSignedInMenu(*ux.currentUser());
        const auto choice = readInteger("Choose an option: ");
        if (!choice) {
            std::cout << "Please enter a number from the menu.\n";
            continue;
        }

        switch (*choice) {
            case 1:
                displayUserSettings(ux, account, preferences, ingredients, catalog);
                break;
            case 2: {
                const auto budget = readNumber("Weekly food budget: $ ");
                if (!budget || !preferences.setBudget(*budget)) {
                    std::cout << "Please enter a non-negative number.\n";
                    break;
                }
                saveUserSettings(ux, account, preferences, ingredients);
                clearGeneratedPlan(mealPlan, grocery, currentPlanMode);
                std::cout << "Weekly budget saved.\n";
                break;
            }
            case 3:
                preferences.enterDietaryPreferences(catalog.getDietaryTags());
                finishFormattedInput();
                saveUserSettings(ux, account, preferences, ingredients);
                clearGeneratedPlan(mealPlan, grocery, currentPlanMode);
                break;
            case 4:
                account.enterFoodAllergies(catalog.getAllergens());
                finishFormattedInput();
                saveUserSettings(ux, account, preferences, ingredients);
                clearGeneratedPlan(mealPlan, grocery, currentPlanMode);
                break;
            case 5:
                ingredients.enterIngredients(catalog.getAllIngredients());
                if (std::cin.fail()) {
                    finishFormattedInput();
                }
                saveUserSettings(ux, account, preferences, ingredients);
                clearGeneratedPlan(mealPlan, grocery, currentPlanMode);
                break;
            case 6:
                handleUploadImage(ux, accountPath);
                break;
            case 7: {
                const auto imagePath = ux.getProfileImagePath(*ux.currentUser());
                std::cout << (imagePath ? "Profile image: " + *imagePath : "No profile image uploaded.") << '\n';
                break;
            }
            case 8:
                handleGenerateMealPlan(
                    mealGenerator,
                    mealPlan,
                    grocery,
                    catalog,
                    account,
                    preferences,
                    ingredients,
                    currentPlanMode
                );
                break;
            case 9:
                if (mealPlan.countMeals() == 0) {
                    std::cout << "Generate a meal plan first.\n";
                } else {
                    mealPlan.display(std::cout, catalog);
                }
                break;
            case 10:
                handleRecipeDetails(mealPlan, catalog);
                break;
            case 11:
                handleReplaceMeal(
                    mealPlan,
                    grocery,
                    catalog,
                    account,
                    preferences,
                    ingredients,
                    currentPlanMode
                );
                break;
            case 12:
                if (mealPlan.countMeals() == 0) {
                    std::cout << "Generate a meal plan first.\n";
                } else {
                    grocery.displayList(catalog);
                    MainHelper::displayBudgetStatus(grocery, preferences, std::cout);
                }
                break;
            case 13:
                closeSession(ux, account, preferences, ingredients, mealPlan, grocery, accountPath);
                currentPlanMode.reset();
                std::cout << "Signed out.\n";
                break;
            case 14:
                closeSession(ux, account, preferences, ingredients, mealPlan, grocery, accountPath);
                currentPlanMode.reset();
                running = false;
                break;
            default:
                std::cout << "Please choose an option from 1 to 14.\n";
        }
    }

    std::cout << "Goodbye!\n";
    return 0;
}
