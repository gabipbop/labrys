// Tests de non-régression du son (§15.6) : chaque scénario de rendu est comparé à ses métriques de référence.
// Changement de son volontaire : `DedaleRender --golden plugins/dedale/Tests/golden/metrics.json`,
// dans un commit séparé et justifié.

#include "../Tools/Scenarios.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("Golden : les rendus de référence n'ont pas bougé", "[golden]")
{
    const auto golden = juce::JSON::parse(juce::File(DEDALE_GOLDEN_FILE));
    REQUIRE(golden.isObject());

    const auto all = render::scenarios();
    REQUIRE(all.size() >= 10);
    for (const auto& scenario : all)
    {
        DYNAMIC_SECTION(scenario.name)
        {
            const auto expected = golden[scenario.name];
            REQUIRE(expected.isObject());

            const auto m = render::measure(
                render::renderScenario(scenario, render::kDefaultSeed, render::kGoldenSeconds));
            REQUIRE(m.finite);
            REQUIRE(m.peakDb <= 0.0);
            REQUIRE(m.rmsDb > -60.0); // pas de silence inattendu
            CHECK(m.peakDb == Approx(static_cast<double>(expected["peak_db"])).margin(0.5));
            CHECK(m.rmsDb == Approx(static_cast<double>(expected["rms_db"])).margin(0.5));
            CHECK(m.dc == Approx(static_cast<double>(expected["dc"])).margin(0.002));
            CHECK(m.crossingsPerSecond ==
                  Approx(static_cast<double>(expected["crossings_per_second"])).epsilon(0.03).margin(5.0));
        }
    }
}

int main(int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI juceInitialiser; // l'APVTS a besoin du MessageManager
    return Catch::Session().run(argc, argv);
}
