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

## Install Qt 6

The easiest option on all systems is the [Qt Online Installer](https://www.qt.io/download-qt-installer-oss). During installation, select a Qt 6 version for desktop development.

### Windows

1. Download and run the Windows Qt Online Installer.
2. Select a Qt 6 desktop kit. The MinGW 64-bit kit is the simplest choice if you do not already use Visual Studio.
3. Complete the installation and keep the selected compiler and Qt kit together.

### macOS

1. Install the Apple command-line tools with `xcode-select --install`.
2. Download and run the macOS Qt Online Installer.
3. Select a Qt 6 kit for macOS desktop development.

### Ubuntu/Debian Linux

Install Qt 6 Widgets and its development tools:

```bash
sudo apt update
sudo apt install qt6-base-dev qt6-base-dev-tools
```

Other Linux distributions can use their package manager or the Qt Online Installer. See the [official Qt installation guide](https://doc.qt.io/qt-6/get-and-install-qt.html) for additional options.

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
