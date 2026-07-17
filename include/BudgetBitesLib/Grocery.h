#pragma once   // prevents double-inclusion

#include <string>
#include <vector>

struct GroceryItem {
    std::string name;
    double quantity;
    std::string unit;
    double pricePerUnit;
};

class Grocery {
public:
    void addItem(const std::string& name,
                 double quantity,
                 const std::string& unit,
                 double pricePerUnit);

    void removeItem(const std::string& name);

    double calculateTotal() const;

    bool isWithinBudget(double weeklyBudget) const;

    void displayList() const;

    const std::vector<GroceryItem>& getItems() const;

private:
    std::vector<GroceryItem> items;
};