#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Grocery.h"
#include "BudgetBitesLib/Infrastructure.h"
#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/RecipeDataBase.h"
#include "BudgetBitesLib/UX.h"
#include "BudgetBitesLib/Ingredients.h"
#include "BudgetBitesLib/MealGenerator.h"
#include "BudgetBitesLib/Preferences.h"

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace {

constexpr const char* kUsersFilePath = "users.dat";

// Reads a line of input, discarding anything left in the stream on failure
// (e.g. if the user typed letters where a menu number was expected).
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// Reads a password without echoing it to the screen.
// On Windows, characters are masked with '*' as they're typed.
// On POSIX systems (Linux/macOS), terminal echo is disabled entirely for
// the duration of input, so nothing appears on screen while typing.
std::string readPassword(const std::string& prompt) {
    std::cout << prompt;
    std::string password;

#ifdef _WIN32
    char ch;
    while ((ch = static_cast<char>(_getch())) != '\r' && ch != '\n') {
        if (ch == '\b') { // backspace
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        } else {
            password.push_back(ch);
            std::cout << '*';
        }
    }
#else
    termios oldSettings{};
    tcgetattr(STDIN_FILENO, &oldSettings);
    termios newSettings = oldSettings;
    newSettings.c_lflag &= ~static_cast<tcflag_t>(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    std::getline(std::cin, password);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
#endif

    std::cout << std::endl;
    return password;
}

void printMenu() {
    std::cout << "\n=== BudgetBites Account Menu ===\n"
              << "1. Register new user\n"
              << "2. Sign in\n"
              << "3. Upload profile image\n"
              << "4. View profile image path\n"
              << "5. Enter food allergies\n"
              << "6. View food allergies\n"
              << "7. Enter available ingredients\n"
              << "8. View Available ingredients\n"
              << "9. Add dietary preference\n"
              << "10. Set weekly budget\n"
              << "11. View Preferences and budget\n"
              << "12. Generate weekly meal plan\n"
              << "13. View weekly Meal plan\n"
              << "14. Sign out\n"
              << "15. Exit\n"
              << "Choose an option: ";
}

void handleRegister(UX& ux) {
    std::string username = readLine("Choose a username: ");
    std::string password = readPassword(
        "Choose a password (8+ chars, upper, lower, digit, special char): ");

    if (ux.registerUser(username, password)) {
        ux.saveToFile(kUsersFilePath);
        std::cout << "Registered successfully. You can now sign in.\n";
    } else {
        std::cout << "Registration failed: the username may already be taken, "
                     "or the password didn't meet the complexity requirements.\n";
    }
}

void handleSignIn(UX& ux) {
    std::string username = readLine("Username: ");
    std::string password = readPassword("Password: ");

    if (ux.signIn(username, password)) {
        std::cout << "Signed in as " << username << ".\n";
    } else {
        std::cout << "Sign-in failed: incorrect username or password.\n";
    }
}

void handleUploadImage(UX& ux) {
    if (!ux.isSignedIn()) {
        std::cout << "You must sign in before uploading a profile image.\n";
        return;
    }

    std::string path = readLine("Path to image file (.png/.jpg/.jpeg/.bmp/.gif): ");
    if (ux.uploadProfileImage(*ux.currentUser(), path)) {
        ux.saveToFile(kUsersFilePath);
        std::cout << "Profile image uploaded successfully.\n";
    } else {
        std::cout << "Upload failed: check that the file exists and has a "
                     "supported image extension.\n";
    }
}

void handleViewImage(const UX& ux) {
    if (!ux.isSignedIn()) {
        std::cout << "You must sign in first.\n";
        return;
    }

    auto path = ux.getProfileImagePath(*ux.currentUser());
    if (path) {
        std::cout << "Profile image: " << *path << "\n";
    } else {
        std::cout << "No profile image has been uploaded yet.\n";
    }
}
    //allows the signed in user to enter/update food allergies
    void handleEnterAllergies(UX& ux) {
    if (!ux.isSignedIn()) {
        std::cout << "You must sign in before entering allergies.\n";
        return;
    }

    ux.currentUser()->enterFoodAllergies();

    ux.saveToFile(kUsersFilePath);
}

    //Displays allergies saved for the sign in user
    void handleViewAllergies( UX& ux) {
    if (!ux.isSignedIn()) {
        std::cout << "You must sign in before viewing allergies.\n";
        return;
    }

    ux.currentUser()->displayFoodAllergies();
}
void handleAddIngredient(Ingredients& ingredients) {
    std::string ingredient =
        readLine("Enter an available ingredient: ");
    if (ingredients.addIngredient(ingredient)) {
        std::cout << "Ingredient added.\n";

    }else {
        std::cout << "Ingredient was already entered.\n";
    }
}

void handleViewIngredients(const Ingredients& ingredients) {


    ingredients.displayIngredients();
}

void handleAddPreferences(Preferences& preferences) {
    std::string preference =
        readLine("Enter a dietary preference: ");

    if (preferences.addPreference(preference)) {
        std::cout << "Preference saved.\n";
    }else {
        std::cout << "Preference was empty or saved already.\n";
    }
}


void handleSetBudget(Preferences& preferences) {
    std::string budgetInput = readLine("Enter an budget: $ ");

    try {
        double budget = std::stod(budgetInput);


        if (budget <0.0) {
            std::cout << "Budget can't be negative.\n";
            return;
        }


        preferences.setBudget(budget);
        std::cout << "Weekly budget saved.\n";
    } catch (...) {
        std::cout << "Please enter a valid number.\n";
    }
}

void handleViewPreferences(const Preferences& preferences) {
    std::cout <<"\nSaved preferences:\n";

    const std::vector<std::string>& savedPreferences =
        preferences.getPreferences();

    if (savedPreferences.empty()) {
        std::cout << "No preferences saved.\n";
    } else {
        for (std::size_t index=0; index < savedPreferences.size(); ++index) {
            std::cout << index + 1 << ". "<<savedPreferences[index]<<"\n";

        }
    }
    std::cout << "Weekly budget: $"
              << preferences.getBudget() << "\n";


}

void handleGenerateMealPlan(
    MealGenerator& generator, MealPlan& mealPlan) {

    std::vector<std::string> recipes = {
        "Oatmeal",
        "Egg Sandwich",
        "Chicken Salad",
        "Rice Bowl",
        "Vegetable Pasta",
        "Turkey Wrap",
        "Bean Tacos"

    };

    generator.generateWeeklyMealPlan(
        mealPlan, recipes);
    std::cout << "Weekly meal plan generated.\n";
}

void handleViewMealPlan(const MealPlan& mealPlan) {
    mealPlan.display(std::cout);
}
} // namespace



int main() {
    UX ux;
    Ingredients ingredients;
    Preferences preferences;
    MealPlan mealPlan;
    MealGenerator mealGenerator;


    if (ux.loadFromFile(kUsersFilePath)) {
        std::cout << "Loaded existing accounts from " << kUsersFilePath << ".\n";
    }

    bool running = true;

    while (running) {
        printMenu();
        std::string choiceInput = readLine("");

        int choice = -1;
        try {
            choice = std::stoi(choiceInput);
        } catch (...) {
            std::cout << "Please enter a number from the menu.\n";
            continue;
        }

        switch (choice) {
            case 1:
                handleRegister(ux);
                break;
            case 2:
                handleSignIn(ux);
                break;
            case 3:
                handleUploadImage(ux);
                break;
            case 4:
                handleViewImage(ux);
                break;
            case 5:
                handleEnterAllergies(ux);
                break;
            case 6:
                handleViewAllergies(ux);
                break;
            case 7:
                handleAddIngredient(ingredients);
                break;

            case 8:
                handleViewIngredients(ingredients);
                break;

            case 9:
                handleAddPreferences(preferences);
                break;

            case 10:
                handleSetBudget(preferences);
                break;

            case 11:
                handleViewPreferences(preferences);
                break;

            case 12:
                handleGenerateMealPlan(
                    mealGenerator,
                    mealPlan
                );
                break;

            case 13:
                handleViewMealPlan(mealPlan);
                break;

            case 14:
                ux.signOut();
                std::cout << "Signed out.\n";
                break;
            case 15:
                ux.saveToFile(kUsersFilePath);
                running = false;
                break;
            default:
                std::cout << "Unknown option. Please choose 1-15.\n";
        }
    }

    std::cout << "Goodbye!\n";
    return 0;

}