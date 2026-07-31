//
// Created by Kezia Saint-Hilaire on 7/29/2026.
//

#pragma once

#ifndef BUDGETBITES_PREFERENCES_H
#define BUDGETBITES_PREFERENCES_H

#include <string>
#include <vector>
#include <algorithm>
class Preferences {

public:
    bool addPreference( const std::string& preference);


    const std::vector<std::string>& getPreferences() const;
    void setBudget(double budget);


    double getBudget() const;


private:

    std::vector<std::string> preferences_;
    double budget_ = 0.0;


};


#endif //BUDGETBITES_PREFERENCES_H