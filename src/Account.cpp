#include "BudgetBitesLib/Account.h"

#include <algorithm>
#include <iostream>


using namespace std;


void Account::enterFoodAllergies() {
    RecipeDataBase catalog;
    if (!catalog.isLoaded()) {
        cout << "Unable to load allergen options.\n";
        return;
    }
    enterFoodAllergies(catalog.getAllergens());
}

void Account::enterFoodAllergies(const vector<Allergen>& availableAllergens) {
    allergenIds_.clear();
    allergies.clear();

    cout << "\nAvailable Food Allergens:\n";
    for (const Allergen& allergen : availableAllergens) {
        cout << allergen.allergenId << ". " << allergen.displayName << '\n';
    }

    cout << "Enter allergen IDs one at a time, or 0 when finished.\n";
    int selectedId;
    while (cout << "Allergen ID: " && cin >> selectedId && selectedId != 0) {
        const auto found = find_if(availableAllergens.begin(), availableAllergens.end(), [&](const Allergen& allergen) {
            return allergen.allergenId == selectedId;
        });
        if (found == availableAllergens.end()) {
            cout << "That allergen ID is not in the list.\n";
            continue;
        }
        if (find(allergenIds_.begin(), allergenIds_.end(), selectedId) == allergenIds_.end()) {
            allergenIds_.push_back(selectedId);
        }
    }
    cout << "\nFood allergies saved!\n";
}

void Account::displayFoodAllergies() const {
    RecipeDataBase catalog;
    if (!catalog.isLoaded()) {
        cout << "Unable to load allergen names.\n";
        return;
    }
    displayFoodAllergies(catalog.getAllergens());
}

void Account::displayFoodAllergies(const vector<Allergen>& availableAllergens) const {
    cout << "\nFood Allergies:\n";
    if (allergenIds_.empty() && allergies.empty()) {
        cout << "None entered.\n";
        return;
    }

    // Current data uses IDs; old text values remain as a temporary fallback.
    if (!allergenIds_.empty()) {
        for (const int allergenId : allergenIds_) {
            const auto found = find_if(availableAllergens.begin(), availableAllergens.end(), [&](const Allergen& allergen) {
                return allergen.allergenId == allergenId;
            });
            if (found != availableAllergens.end()) {
                cout << "- " << found->displayName << '\n';
            }
        }
        return;
    }
    for (const string& allergy : allergies) {
        cout << "- " << allergy << '\n';
    }
}

vector<string> Account::getAllergies() const {
    return allergies;
}

void Account::setAllergies(const vector<string>& loadedAllergies) {
    allergies = loadedAllergies;
    allergenIds_.clear();
}

void Account::setAllergenIds(const vector<int>& allergenIds) {
    allergenIds_ = allergenIds;
    allergies.clear();
}

const vector<int>& Account::getAllergenIds() const {
    return allergenIds_;
}
