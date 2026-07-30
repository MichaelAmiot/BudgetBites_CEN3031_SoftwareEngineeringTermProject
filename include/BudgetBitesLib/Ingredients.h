//
// Created by Kezia Saint-Hilaire on 7/30/2026.
//

#ifndef BUDGETBITES_INGREDIENTS_H
#define BUDGETBITES_INGREDIENTS_H

#include <iostream>
#include <string>
#include <vector>
class Ingredients {

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


#endif //BUDGETBITES_INGREDIENTS_H#pragma once   // prevents double-inclusion



