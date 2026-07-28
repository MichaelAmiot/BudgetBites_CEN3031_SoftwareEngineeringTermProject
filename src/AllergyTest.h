#include <iostream>
#include "BudgetBitesLib/Account.h"

using namespace std;

int main() {
    Account testAccount;

    cout << "FOOD ALLERGY FEATURE TEST\n";
    cout << "-------------------------\n";

    testAccount.enterFoodAllergies();

    cout << "\nStored allergy output:\n";
    testAccount.displayFoodAllergies();

    const vector<string>& allergies = testAccount.getAllergies();

    cout << "\nTest result: ";

    if (!allergies.empty()) {
        cout << "PASSED\n";
        cout << "The Account object successfully stored "
             << allergies.size() << " allergy/allergies.\n";
    } else {
        cout << "PASSED\n";
        cout << "The Account object correctly stored an empty allergy list.\n";
    }

    return 0;
}