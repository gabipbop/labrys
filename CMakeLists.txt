cmake_minimum_required(VERSION 3.25)

project(LABRYS VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LABRYS_BUILD_TESTS "Construit les cibles de tests" ON)
option(LABRYS_WARNINGS_AS_ERRORS "Traite les warnings comme des erreurs" OFF)
option(LABRYS_SANITIZE "AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

if(LABRYS_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

include(libs/ariane/cmake/SuiteIdentity.cmake)
include(libs/ariane/cmake/ArianeWarnings.cmake)

if(LABRYS_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    enable_testing()
endif()

add_subdirectory(libs/ariane)
add_subdirectory(plugins/dedale)
