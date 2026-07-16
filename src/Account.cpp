#include "Account.h"
#include <iostream>
#include <limits>


using namespace std;


void Account::enterFoodAllergies() {

//clearing the previous allergies before adding more
allergies.clear();

int num;



cout << '\nHow many food allergies would you like to enter? ";

cin >> num;



//remove leftover newline
cin.ignore(numeric_limits<streamsize>::max(), '\n');

for (int i = 0;  i < num; i++) {


    string allergy;


    cout << "Enter allergy #" << i + 1 << ": ";
    getline (cin, allergy);


    allergies.push_back(allergy); //ADD ALLERGY TO LIST

    }
    cout << "\nFood allergies saved!\n";

}




void Account::displayFoodAllergies()  const{

    cout<< "\nFood Allergies:\n";


    if (allergies.empty()) {

        cout << "None entered. \n";

        return;
    }



    for (const auto &item : allergies) {



        cout << "- " << item << endl;



    }
}

vector<string> Account::getAllergies() const {


    return allergies;



}
