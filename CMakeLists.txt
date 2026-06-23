cmake_minimum_required(VERSION 3.20)
project(BudgetBites)

set(CMAKE_CXX_STANDARD 17)

# Makes #include "BudgetBitesLib/Account.h" work
include_directories(include)

add_subdirectory(src)