//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//

#include "../include/BudgetBitesLib/Preferences.h"
#include <algorithm>

//save dietary pref]

bool Preferences::addPreference(const std::string& preference) {

    if (preference.empty()) {
        return false;



    }
//no duplicates like 4 vegetarians
    if (std::find(preferences_.begin(),



                  preferences_.end(),


                  preference) != preferences_.end()) {
        return false;
                  }

    preferences_.push_back(preference);

    return true;
}

//return

const std::vector<std::string>& Preferences::getPreferences() const {

    return preferences_;
}


//save budget


void Preferences::setBudget(double budget) {

    budget_ = budget;



}


//return saved budget

double Preferences::getBudget() const {

    return budget_;



}

