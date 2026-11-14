#include <catch2/catch_test_macros.hpp>

// Vérifie seulement que la chaîne de tests (CMake + Catch2 + ctest) fonctionne.
// À supprimer dès que les premiers vrais tests (phase 2) existent.
TEST_CASE("La chaîne de tests fonctionne", "[smoke]")
{
    REQUIRE(1 + 1 == 2);
}
