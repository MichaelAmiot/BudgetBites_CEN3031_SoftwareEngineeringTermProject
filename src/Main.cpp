#include "BudgetBitesLib/Account.h"
#include "BudgetBitesLib/Grocery.h"
#include "BudgetBitesLib/Infrastructure.h"
#include "BudgetBitesLib/MealPlan.h"
#include "BudgetBitesLib/RecipeDataBase.h"
#include "BudgetBitesLib/UX.h"

#include <iostream>
#include <limits>
#include <map>
#include <string>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

namespace {

const string usersFilePath = "users.txt";

// keeps each user's Account (allergies) in memory while the program runs.
// Account isn't saved to the file the way UserRepository is, so
// allergies get reset every time the program restarts.
// TODO: figure out a way to save this along with everything else
map<string, Account> accountsByUser;

// prints a prompt and reads a line of input
string readLine(const string& prompt) {
    cout << prompt;
    string line;
    getline(cin, line);
    return line;
}

// reads a password without showing it on screen.
// On Windows it shows * for each character typed.
// On Mac/Linux it just hides the input completely while typing.
string readPassword(const string& prompt) {
    cout << prompt;
    string password;

#ifdef _WIN32
    char ch;
    while ((ch = static_cast<char>(_getch())) != '\r' && ch != '\n') {
        if (ch == '\b') { // backspace
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b";
            }
        } else {
            password.push_back(ch);
            cout << '*';
        }
    }
#else
    termios oldSettings{};
    tcgetattr(STDIN_FILENO, &oldSettings);
    termios newSettings = oldSettings;
    newSettings.c_lflag &= ~static_cast<tcflag_t>(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    getline(cin, password);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
#endif

    cout << endl;
    return password;
}

void printMenu() {
    cout << "\n=== BudgetBites Account Menu ===\n"
         << "1. Register new user\n"
         << "2. Sign in\n"
         << "3. Upload profile image\n"
         << "4. View profile image path\n"
         << "5. Enter food allergies\n"
         << "6. View food allergies\n"
         << "7. Sign out\n"
         << "8. Exit\n"
         << "Choose an option: ";
}

void handleRegister(UX& ux) {
    string username = readLine("Choose a username: ");
    string password = readPassword("Choose a password (8+ chars, upper, lower, digit, special char): ");

    if (ux.registerUser(username, password)) {
        ux.saveToFile(usersFilePath);
        cout << "Registered successfully. You can now sign in.\n";
    } else {
        cout << "Registration failed: the username may already be taken, "
                "or the password didn't meet the complexity requirements.\n";
    }
}

void handleSignIn(UX& ux) {
    string username = readLine("Username: ");
    string password = readPassword("Password: ");

    if (ux.signIn(username, password)) {
        cout << "Signed in as " << username << ".\n";
    } else {
        cout << "Sign-in failed: incorrect username or password.\n";
    }
}

void handleUploadImage(UX& ux) {
    if (!ux.isSignedIn()) {
        cout << "You must sign in before uploading a profile image.\n";
        return;
    }

    string path = readLine("Path to image file (.png/.jpg/.jpeg/.bmp/.gif): ");
    if (ux.uploadProfileImage(*ux.currentUser(), path)) {
        ux.saveToFile(usersFilePath);
        cout << "Profile image uploaded successfully.\n";
    } else {
        cout << "Upload failed: check that the file exists and has a "
                "supported image extension.\n";
    }
}

// takes UX by reference, not const reference, since getProfileImagePath
// isn't a const function anymore
void handleViewImage(UX& ux) {
    if (!ux.isSignedIn()) {
        cout << "You must sign in first.\n";
        return;
    }

    auto path = ux.getProfileImagePath(*ux.currentUser());
    if (path) {
        cout << "Profile image: " << *path << "\n";
    } else {
        cout << "No profile image has been uploaded yet.\n";
    }
}

// lets the signed in user enter/update their food allergies
void handleEnterAllergies(UX& ux) {
    if (!ux.isSignedIn()) {
        cout << "You must sign in before entering allergies.\n";
        return;
    }

    accountsByUser[*ux.currentUser()].enterFoodAllergies();

    ux.saveToFile(usersFilePath);
}

// shows allergies saved for the signed in user
void handleViewAllergies(UX& ux) {
    if (!ux.isSignedIn()) {
        cout << "You must sign in before viewing allergies.\n";
        return;
    }

    accountsByUser[*ux.currentUser()].displayFoodAllergies();
}

} // namespace

int main() {
    UX ux;
    if (ux.loadFromFile(usersFilePath)) {
        cout << "Loaded existing accounts from " << usersFilePath << ".\n";
    }

    bool running = true;

    while (running) {
        printMenu();
        string choiceInput = readLine("");

        int choice = -1;
        try {
            choice = stoi(choiceInput);
        } catch (...) {
            cout << "Please enter a number from the menu.\n";
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
                ux.signOut();
                cout << "Signed out.\n";
                break;
            case 8:
                ux.saveToFile(usersFilePath);
                running = false;
                break;
            default:
                cout << "Unknown option. Please choose 1-8.\n";
        }
    }

    cout << "Goodbye!\n";
    return 0;
}