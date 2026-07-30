//
// Created by micha on 6/17/2026.
//
//Kezia Saint-Hilaire Sprint 1
//

#pragma once


#include <vector>
#include <string>
#ifndef ACCOUNT_H
#define ACCOUNT_H



class Account {

private:
//this vector stores the allergies the user enters
    std::vector<std::string> allergies;

public:


//lets a user enter their allergies
    void enterFoodAllergies();


//Prints the allergies that are being saved!
    void displayFoodAllergies() const;


std::vector<std::string> getAllergies() const;


//used when loading allergies back in from a file, so the loaded
//list replaces whatever (if anything) is currently stored
void setAllergies(const std::vector<std::string>& loadedAllergies);


};


#endif //ACCOUNT_H
