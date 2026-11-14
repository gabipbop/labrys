# DÉDALE : le plugin arrive en phase 1. Pour l'instant, seulement la cible de tests.

if(LABRYS_BUILD_TESTS)
    add_executable(DedaleTests Tests/SmokeTests.cpp)
    target_link_libraries(DedaleTests PRIVATE labrys::ariane ariane_warnings Catch2::Catch2WithMain)
    catch_discover_tests(DedaleTests)
endif()
