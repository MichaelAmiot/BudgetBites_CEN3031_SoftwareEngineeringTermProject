#include "BudgetBitesLib/Grocery.h"
#include <algorithm>
#include <iomanip>
#include <iostream>

void Grocery::addItem(const std::string& name,
                      double quantity,
                      const std::string& unit,
                      double pricePerUnit) {
    if (name.empty() || quantity <= 0 || pricePerUnit < 0) {
        return;
    }

    for (GroceryItem& item : items) {
        if (item.name == name &&
            item.unit == unit &&
            item.pricePerUnit == pricePerUnit) {
            item.quantity += quantity;
            return;
        }
    }

    items.push_back({name, quantity, unit, pricePerUnit});
}

void Grocery::removeItem(const std::string& name) {
    items.erase(
            std::remove_if(
                    items.begin(),
                    items.end(),
                    [&name](const GroceryItem& item) {
                        return item.name == name;
                    }),
            items.end()
    );
}

double Grocery::calculateTotal() const {
    double total = 0.0;

    for (const GroceryItem& item : items) {
        total += item.quantity * item.pricePerUnit;
    }

    return total;
}

bool Grocery::isWithinBudget(double weeklyBudget) const {
    return weeklyBudget >= 0 &&
           calculateTotal() <= weeklyBudget;
}

void Grocery::displayList() const {
    std::cout << "\n=== Grocery List ===\n";

    if (items.empty()) {
        std::cout << "The grocery list is empty.\n";
        return;
    }

    for (const GroceryItem& item : items) {
        const double itemTotal =
                item.quantity * item.pricePerUnit;

        std::cout << item.name << ": "
                  << item.quantity << " "
                  << item.unit << " - $"
                  << std::fixed
                  << std::setprecision(2)
                  << itemTotal << '\n';
    }

    std::cout << "Estimated total: $"
              << std::fixed
              << std::setprecision(2)
              << calculateTotal()
              << '\n';
}

const std::vector<GroceryItem>& Grocery::getItems() const {
    return items;
}