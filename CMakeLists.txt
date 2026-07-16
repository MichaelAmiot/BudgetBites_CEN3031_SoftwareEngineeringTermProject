cmake_minimum_required(VERSION 3.20)
project(BudgetBites)

set(CMAKE_CXX_STANDARD 17)

# Makes #include "BudgetBitesLib/Account.h" work
include_directories(include)

add_subdirectory(src)

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1  # pick current release
)
FetchContent_MakeAvailable(Catch2)

enable_testing()
add_subdirectory(tests)