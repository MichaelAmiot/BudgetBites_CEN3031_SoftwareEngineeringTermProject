# BudgetBites

BudgetBites is a desktop meal-planning application for college students. It creates weekly meal plans and grocery lists
based on a user's budget, dietary preferences, allergens, and available pantry ingredients.

## Features

- Account registration and login
- Dietary, allergen, budget, and pantry settings
- Recipe search and filtering
- Seven-day meal-plan generation
- Automatic grocery-list and cost estimation

## Requirements

- CMake 3.20 or later
- A C++17-compatible compiler
- Qt 6 Widgets
- Git

## Build

Clone the repository and enter the project directory:

```bash
git clone https://github.com/MichaelAmiot/BudgetBites_CEN3031_SoftwareEngineeringTermProject.git
cd BudgetBites_CEN3031_SoftwareEngineeringTermProject
```

Configure and build the project:

```bash
cmake -S . -B build
cmake --build build
```

If CMake cannot find Qt, provide the location of the Qt 6 CMake package:

```bash
cmake -S . -B build -DQt6_DIR="/path/to/Qt/lib/cmake/Qt6"
```

## Run

Run the application from the project directory.

### Graphical application

- Windows: `build\gui\BudgetBitesGUI.exe`
- macOS: `open build/gui/BudgetBitesGUI.app`
- Linux: `./build/gui/BudgetBitesGUI`

Create a new account from the Register page after the application starts.

### Command-line application

- Windows: `build\src\BudgetBites.exe`
- macOS/Linux: `./build/src/BudgetBites`

## Data

The recipe catalog is included in `data/seed`. User accounts and settings are stored locally in `data/local`.
