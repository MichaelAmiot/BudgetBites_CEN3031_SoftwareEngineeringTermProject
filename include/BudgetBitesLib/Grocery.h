#pragma once   // prevents double-inclusion


#ifndef BUDGETBITESLIB_GROCERY_H
#define BUDGETBITESLIB_GROCERY_H

#include <iostream>
#include <string>
#include <vector>


class Grocery {

public:
    bool addIngredient(const std::string& ingredient);

    bool removeIngredient(const std::string& ingredient);

    bool hasIngredient(const std::string& ingredient) const;


    void displayIngredients() const;

    std::size_t countIngredients() const;



    void clearIngredients();


private:

    std::vector<std::string> ingredients_;

};

#endif




