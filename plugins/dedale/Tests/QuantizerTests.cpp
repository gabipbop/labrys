#include "Sequencer/Quantizer.h"

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using Catch::Approx;
using dedale::quantize;
using dedale::scaleForMode;

namespace
{
bool isInScale(float volts, int mode, int root)
{
    const auto& scale = scaleForMode(mode);
    const auto semitone = static_cast<int>(std::lround(static_cast<double>(volts) * 12.0)) - root;
    const auto pitchClass = ((semitone % 12) + 12) % 12;
    return std::find(scale.degrees.begin(), scale.degrees.begin() + scale.size, pitchClass) !=
           scale.degrees.begin() + scale.size;
}

// Distance (en demi-tons) au degré le plus proche, par force brute sur toutes les octaves utiles.
double nearestDistance(float volts, int mode, int root)
{
    const auto& scale = scaleForMode(mode);
    const auto semitones = static_cast<double>(volts) * 12.0 - root;
    auto best = 1000.0;
    for (int octave = -7; octave <= 7; ++octave)
        for (int i = 0; i < scale.size; ++i)
            best = std::min(best,
                            std::abs(semitones - (octave * 12 + scale.degrees[static_cast<std::size_t>(i)])));
    return best;
}
} // namespace

TEST_CASE("Quantizer : la table des gammes est bien formée", "[quant]")
{
    REQUIRE(scaleForMode(0).size == 0);
    for (int mode = 1; mode < dedale::kNumQuantizerModes; ++mode)
    {
        const auto& scale = scaleForMode(mode);
        REQUIRE(scale.size >= 4);
        REQUIRE(scale.degrees[0] == 0);
        for (int i = 1; i < scale.size; ++i)
        {
            REQUIRE(scale.degrees[static_cast<std::size_t>(i)] >
                    scale.degrees[static_cast<std::size_t>(i - 1)]);
            REQUIRE(scale.degrees[static_cast<std::size_t>(i)] < 12);
        }
    }

    const auto& major = scaleForMode(2);
    REQUIRE(major.size == 7);
    REQUIRE(std::equal(major.degrees.begin(), major.degrees.begin() + 7,
                       std::array<int, 7>{0, 2, 4, 5, 7, 9, 11}.begin()));
    REQUIRE(scaleForMode(1).size == 12); // chromatique
    REQUIRE(scaleForMode(15).size == 4); // quads
}

TEST_CASE("Quantizer : le mode 1 laisse passer la tension", "[quant]")
{
    for (float v : {-5.0f, -1.2345f, 0.0f, 0.0417f, 3.3333f, 5.0f})
        REQUIRE(quantize(v, 0, 7) == v);
}

TEST_CASE("Quantizer : chaque mode rend le degré le plus proche de sa gamme", "[quant]")
{
    for (int mode = 1; mode < dedale::kNumQuantizerModes; ++mode)
    {
        for (int root : {0, 5, -3})
        {
            for (float v = -5.0f; v <= 5.0f; v += 0.0137f)
            {
                const auto q = quantize(v, mode, root);
                INFO("mode " << mode << " root " << root << " v " << v << " q " << q);
                REQUIRE(isInScale(q, mode, root));
                REQUIRE(std::abs(static_cast<double>(q - v)) * 12.0 <= nearestDistance(v, mode, root) + 1e-3);
                REQUIRE(quantize(q, mode, root) == Approx(q).margin(1e-5)); // idempotent
            }
        }
    }
}

TEST_CASE("Quantizer : valeurs limites", "[quant]")
{
    for (int mode = 1; mode < dedale::kNumQuantizerModes; ++mode)
    {
        for (float v : {-5.0f, 5.0f})
        {
            const auto q = quantize(v, mode, 0);
            REQUIRE(std::isfinite(q));
            REQUIRE(q == Approx(v).margin(1e-5)); // ±5 V = fondamentale, présente dans toutes les gammes
        }
    }
    // Modes hors bornes : ramenés dans 0..15, sans crash.
    REQUIRE(quantize(0.3f, -4, 0) == 0.3f);
    REQUIRE(std::isfinite(quantize(0.3f, 99, 0)));
}

TEST_CASE("Quantizer : exemples connus", "[quant]")
{
    REQUIRE(quantize(0.04f, 1, 0) == Approx(0.0f).margin(1e-6)); // 0,48 demi-ton → do
    REQUIRE(quantize(0.05f, 1, 0) == Approx(1.0f / 12.0f));      // 0,6 demi-ton → do dièse
    REQUIRE(quantize(0.1f, 2, 0) == Approx(2.0f / 12.0f));       // 1,2 demi-ton en majeur → ré
    REQUIRE(quantize(-0.05f, 2, 0) ==
            Approx(-1.0f / 12.0f));                 // sous la fondamentale → si de l'octave d'en dessous
    REQUIRE(quantize(0.97f, 2, 0) == Approx(1.0f)); // remonte à l'octave
    REQUIRE(quantize(0.2f, 15, 0) == Approx(3.0f / 12.0f)); // quads : 2,4 demi-tons → tierce mineure
}

TEST_CASE("Quantizer : la fondamentale se transpose", "[quant]")
{
    // Ré majeur contient do dièse (1 demi-ton), pas do majeur.
    REQUIRE(quantize(1.02f / 12.0f, 2, 2) == Approx(1.0f / 12.0f));
    REQUIRE(quantize(1.02f / 12.0f, 2, 0) != Approx(1.0f / 12.0f));
    // Une transposition d'une octave ne change rien.
    for (float v = -2.0f; v <= 2.0f; v += 0.05f)
        REQUIRE(quantize(v, 5, 12) == Approx(quantize(v, 5, 0)).margin(1e-5));
}
