#include "Sequencer/Clock.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <vector>

using dedale::HostClock;
using dedale::InternalClock;

namespace
{
constexpr double sampleRate = 48000.0;

// Rend les positions (en échantillons) des ticks d'une lecture continue découpée en blocs.
std::vector<long> hostTicks(HostClock& clock, double bpm, int division, long totalSamples, int blockSize,
                            double startPpq = 0.0)
{
    const auto ppqPerSample = bpm / 60.0 / sampleRate;
    std::vector<long> ticks;
    for (long start = 0; start < totalSamples; start += blockSize)
    {
        clock.beginBlock(startPpq + static_cast<double>(start) * ppqPerSample, ppqPerSample);
        const auto length = std::min<long>(blockSize, totalSamples - start);
        for (long i = 0; i < length; ++i)
            if (clock.processSample(division))
                ticks.push_back(start + i);
    }
    return ticks;
}

void requireGrid(const std::vector<long>& ticks, long period, std::size_t count)
{
    REQUIRE(ticks.size() == count);
    for (std::size_t i = 0; i < ticks.size(); ++i)
        REQUIRE(std::labs(ticks[i] - static_cast<long>(i) * period) <= 1); // à l'échantillon près
}
} // namespace

TEST_CASE("Horloge interne : un tick par pas, le premier tout de suite", "[clock]")
{
    InternalClock clock;
    clock.prepare(sampleRate);

    std::vector<long> ticks;
    for (long i = 0; i < 48000 * 2 - 100; ++i) // on s'arrête juste avant le 5e tick
        if (clock.processSample(120.0))        // 2 pas par seconde
            ticks.push_back(i);
    requireGrid(ticks, 24000, 4);

    clock.reset();
    REQUIRE(clock.processSample(120.0));
    REQUIRE_FALSE(clock.processSample(120.0));
}

TEST_CASE("Horloge interne : bornes du tempo", "[clock]")
{
    InternalClock clock;
    clock.prepare(sampleRate);
    int fast = 0;
    for (long i = 0; i < 48000 * 10 - 100; ++i)
        fast += clock.processSample(600.0) ? 1 : 0;
    REQUIRE(fast == 100);

    clock.reset();
    int slow = 0;
    for (long i = 0; i < 48000 * 11; ++i)
        slow += clock.processSample(10.0) ? 1 : 0;
    REQUIRE(slow == 2);
}

TEST_CASE("Horloge hôte : ticks sample-accurate sur la grille PPQ", "[clock]")
{
    // 120 BPM : une double croche = 0,125 s = 6000 échantillons.
    for (int blockSize : {32, 512, 480, 4096})
    {
        HostClock clock;
        requireGrid(hostTicks(clock, 120.0, 1, 48000, blockSize), 6000, 48000 / 6000);
    }
}

TEST_CASE("Horloge hôte : la division allonge le pas", "[clock]")
{
    HostClock quarter;
    requireGrid(hostTicks(quarter, 120.0, 4, 96000, 512), 24000, 4);

    HostClock bar;
    requireGrid(hostTicks(bar, 120.0, 16, 96000 * 2, 512), 96000, 2);
}

TEST_CASE("Horloge hôte : départ entre deux pas, saut et reprise", "[clock]")
{
    HostClock clock;

    SECTION("départ au milieu d'un pas : pas de tick avant la prochaine double croche")
    {
        const auto ticks = hostTicks(clock, 120.0, 1, 12000, 512, 0.125); // PPQ 0,125 = moitié de pas
        REQUIRE(ticks.size() == 2);
        REQUIRE(std::labs(ticks[0] - 3000) <= 1);
        REQUIRE(std::labs(ticks[1] - 9000) <= 1);
    }

    SECTION("boucle : le retour sur un temps retombe sur un tick")
    {
        REQUIRE(hostTicks(clock, 120.0, 1, 6144, 512).size() == 2); // ticks à 0 et 6000
        const auto afterJump = hostTicks(clock, 120.0, 1, 6144, 512, 4.0);
        REQUIRE(afterJump.size() == 2);
        REQUIRE(afterJump[0] == 0);
    }

    SECTION("arrêt puis reprise au même endroit : le temps est rejoué")
    {
        REQUIRE(hostTicks(clock, 120.0, 1, 512, 512).size() == 1);
        clock.reset();
        const auto again = hostTicks(clock, 120.0, 1, 512, 512);
        REQUIRE(again.size() == 1);
        REQUIRE(again[0] == 0);
    }

    SECTION("position négative (pré-roll)")
    {
        const auto ticks = hostTicks(clock, 120.0, 1, 12000, 512, -0.125);
        REQUIRE(ticks.size() == 2);
        REQUIRE(std::labs(ticks[0] - 3000) <= 1);
    }
}

TEST_CASE("Horloge MIDI : 24 impulsions par noire, division en doubles croches", "[clock][midi]")
{
    dedale::MidiClock clock;
    clock.prepare(sampleRate);
    REQUIRE_FALSE(clock.isActive()); // rien reçu : l'horloge interne ou hôte garde la main

    SECTION("division 1 : un tick toutes les 6 impulsions, dès la première")
    {
        int ticks = 0;
        std::vector<int> positions;
        for (int pulse = 0; pulse < 48; ++pulse)
        {
            if (clock.onPulse(1))
            {
                ++ticks;
                positions.push_back(pulse);
            }
        }
        REQUIRE(ticks == 8); // deux noires = 8 doubles croches
        REQUIRE(positions[0] == 0);
        REQUIRE(positions[1] == 6);
        REQUIRE(clock.isActive());
    }

    SECTION("division 4 : une noire par pas ; division 16 : une ronde")
    {
        int quarters = 0;
        for (int pulse = 0; pulse < 96; ++pulse)
            quarters += clock.onPulse(4) ? 1 : 0;
        REQUIRE(quarters == 4);

        clock.restart();
        int bars = 0;
        for (int pulse = 0; pulse < 96 * 2; ++pulse)
            bars += clock.onPulse(16) ? 1 : 0;
        REQUIRE(bars == 2);
    }

    SECTION("Start : la prochaine impulsion est un tick")
    {
        for (int pulse = 0; pulse < 4; ++pulse)
            clock.onPulse(1);
        REQUIRE_FALSE(clock.onPulse(1));
        clock.restart();
        REQUIRE(clock.onPulse(1));
    }

    SECTION("Start arme l'horloge MIDI avant même la première impulsion")
    {
        clock.restart();
        REQUIRE(clock.isActive());
    }

    SECTION("silence de 5 s : l'horloge MIDI rend la main")
    {
        clock.onPulse(1);
        for (long i = 0; i < 48000 * 5 - 10; ++i)
            clock.advance();
        REQUIRE(clock.isActive());
        for (int i = 0; i < 20; ++i)
            clock.advance();
        REQUIRE_FALSE(clock.isActive());
        clock.onPulse(1);
        REQUIRE(clock.isActive());
    }
}
