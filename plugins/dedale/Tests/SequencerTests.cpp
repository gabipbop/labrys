#include "Sequencer/GenerativeSequencer.h"

#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

using Catch::Approx;
using dedale::GenerativeSequencer;
using dedale::kMaxSteps;

namespace
{
// Séquenceur en marche, non quantifié, CV RANGE à fond : cv() rend la tension stockée telle quelle.
GenerativeSequencer makeSequencer(std::uint32_t seed = 1234)
{
    GenerativeSequencer seq(seed);
    seq.setQuantizerMode(0);
    seq.setRunning(true);
    return seq;
}

// Lane 0 : pas pairs actifs, tensions 0,1 × (index + 1). Lane 1 : pas 0, 3, 4 actifs, tensions négatives.
void loadPattern(GenerativeSequencer& seq)
{
    for (int i = 0; i < kMaxSteps; ++i)
    {
        seq.setStep(0, i, i % 2 == 0, 0.1f * static_cast<float>(i + 1));
        seq.setStep(1, i, i == 0 || i == 3 || i == 4, -0.1f * static_cast<float>(i + 1));
    }
}

struct StepOutput
{
    bool trigger;
    float cv;
    bool operator==(const StepOutput&) const = default;
};

std::vector<StepOutput> run(GenerativeSequencer& seq, int lane, int ticks)
{
    std::vector<StepOutput> out;
    for (int i = 0; i < ticks; ++i)
    {
        const auto trigger = seq.tick(lane);
        out.push_back({trigger, seq.cv(lane)});
    }
    return out;
}

std::array<bool, kMaxSteps> bitsOf(const GenerativeSequencer& seq, int lane)
{
    return seq.lane(lane).bit;
}
} // namespace

TEST_CASE("Séquenceur : à l'arrêt, un tick ne fait rien", "[seq]")
{
    GenerativeSequencer seq(1);
    loadPattern(seq);
    REQUIRE_FALSE(seq.tick(0));
    REQUIRE(seq.lane(0).playHead == 0);
}

TEST_CASE("Séquenceur : lecture, trigger et sample & hold", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);

    const auto out = run(seq, 0, 9);
    // Le premier tick joue le pas 1, puis les pas pairs déclenchent, les impairs tiennent la CV.
    REQUIRE(out[0] == StepOutput{true, 0.1f});
    REQUIRE(out[1] == StepOutput{false, 0.1f});
    REQUIRE(out[2] == StepOutput{true, 0.3f});
    REQUIRE(out[7] == StepOutput{false, 0.7f});
    REQUIRE(out[8] == StepOutput{true, 0.1f}); // retour au pas 1
    REQUIRE(seq.lane(0).playHead == 0);
}

TEST_CASE("Séquenceur : corrupt = 0 répète exactement la séquence sur 1000 cycles", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);
    seq.pressLength(1); // lane 1 sur 7 pas : les deux lanes ne bouclent pas ensemble

    const auto first0 = run(seq, 0, 8);
    const auto first1 = run(seq, 1, 7);
    for (int cycle = 0; cycle < 1000; ++cycle)
    {
        REQUIRE(run(seq, 0, 8) == first0);
        REQUIRE(run(seq, 1, 7) == first1);
    }
}

TEST_CASE("Séquenceur : RESET rejoue le pas 1 et garde l'offset du write head", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);
    seq.shiftWriteHead(0);
    run(seq, 0, 4);
    REQUIRE(seq.lane(0).playHead == 3);

    seq.reset();
    REQUIRE(seq.lane(0).playHead == 0);
    REQUIRE(seq.lane(0).writeOffset == 1);
    REQUIRE(seq.tick(0));
    REQUIRE(seq.cv(0) == 0.1f);
    REQUIRE(seq.lane(0).playHead == 0);
    seq.tick(0);
    REQUIRE(seq.lane(0).playHead == 1);
}

TEST_CASE("Séquenceur : ADVANCE avance les deux lanes, seulement à l'arrêt", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);

    seq.advance();
    REQUIRE(seq.lane(0).playHead == 0); // en lecture : inactif

    seq.setRunning(false);
    const auto triggers = seq.advance();
    REQUIRE(seq.lane(0).playHead == 1);
    REQUIRE(seq.lane(1).playHead == 1);
    REQUIRE_FALSE(triggers[0]);
    REQUIRE_FALSE(triggers[1]);
    REQUIRE(seq.advance()[0]); // pas 3 de la lane 0 : actif
    REQUIRE(seq.writeHead(0) == 2);
}

TEST_CASE("Séquenceur : LENGTH boucle 8 → 1 → 8", "[seq]")
{
    auto seq = makeSequencer();
    for (int expected = 7; expected >= 1; --expected)
    {
        seq.pressLength(0);
        REQUIRE(seq.lane(0).length == expected);
    }
    seq.pressLength(0);
    REQUIRE(seq.lane(0).length == 8);
    REQUIRE(seq.lane(1).length == 8); // l'autre lane n'a pas bougé

    seq.pressLength(0);
    seq.pressLength(0);
    seq.resetLength(0);
    REQUIRE(seq.lane(0).length == 8);
}

TEST_CASE("Séquenceur : une séquence raccourcie boucle sur sa longueur", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);
    for (int i = 0; i < 5; ++i)
        seq.pressLength(0); // longueur 3
    const auto out = run(seq, 0, 6);
    REQUIRE(out[0].cv == 0.1f);
    REQUIRE(out[2].cv == 0.3f);
    REQUIRE(out[3] == out[0]);
    REQUIRE(out[5] == out[2]);
}

TEST_CASE("Séquenceur : BIT SHIFT tourne bits et tensions dans la longueur courante", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);
    const auto original = seq.lane(0);

    SECTION("un pas vers la droite, le dernier pas revient au pas 1")
    {
        seq.bitShift(0);
        REQUIRE(seq.lane(0).bit[0] == original.bit[7]);
        REQUIRE(seq.lane(0).voltage[0] == original.voltage[7]);
        REQUIRE(seq.lane(0).bit[1] == original.bit[0]);
        REQUIRE(seq.lane(0).voltage[1] == original.voltage[0]);
    }

    SECTION("rotation puis rotation inverse = identité")
    {
        for (int n = 1; n <= 11; ++n)
        {
            for (int i = 0; i < n; ++i)
                seq.bitShift(0);
            seq.undoBitShift(0);
            REQUIRE(seq.lane(0).bit == original.bit);
            REQUIRE(seq.lane(0).voltage == original.voltage);
        }
    }

    SECTION("autant de rotations que de pas = identité")
    {
        for (int i = 0; i < 8; ++i)
            seq.bitShift(0);
        REQUIRE(seq.lane(0).bit == original.bit);
        REQUIRE(seq.lane(0).voltage == original.voltage);
    }

    SECTION("les pas au-delà de la longueur ne bougent pas")
    {
        for (int i = 0; i < 3; ++i)
            seq.pressLength(0); // longueur 5
        seq.bitShift(0);
        REQUIRE(seq.lane(0).voltage[0] == original.voltage[4]);
        REQUIRE(seq.lane(0).voltage[4] == original.voltage[3]);
        for (std::size_t i = 5; i < 8; ++i)
        {
            REQUIRE(seq.lane(0).bit[i] == original.bit[i]);
            REQUIRE(seq.lane(0).voltage[i] == original.voltage[i]);
        }
        seq.undoBitShift(0);
        REQUIRE(seq.lane(0).voltage == original.voltage);
    }
}

TEST_CASE("Séquenceur : BIT FLIP agit au write head", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);
    run(seq, 0, 2); // play head sur le pas 2 (index 1, éteint)

    SECTION("off → on : nouvelle tension aléatoire ; on → off : tension gardée en mémoire")
    {
        seq.setBitFlipGate(0, true);
        seq.setBitFlipGate(0, false);
        REQUIRE(seq.lane(0).bit[1]);
        const auto fresh = seq.lane(0).voltage[1];
        REQUIRE(fresh != 0.2f);
        REQUIRE(fresh >= -5.0f);
        REQUIRE(fresh < 5.0f);

        seq.setBitFlipGate(0, true);
        seq.setBitFlipGate(0, false);
        REQUIRE_FALSE(seq.lane(0).bit[1]);
        REQUIRE(seq.lane(0).voltage[1] == fresh);
    }

    SECTION("une porte qui reste haute ne flippe qu'une fois sans tick")
    {
        seq.setBitFlipGate(0, true);
        seq.setBitFlipGate(0, true);
        REQUIRE(seq.lane(0).bit[1]);
    }

    SECTION("avec un offset, le flip tombe sur le pas décalé")
    {
        seq.shiftWriteHead(0);
        seq.shiftWriteHead(0);
        REQUIRE(seq.writeHead(0) == 3);
        seq.setBitFlipGate(0, true);
        REQUIRE(seq.lane(0).bit[3]);
        REQUIRE_FALSE(seq.lane(0).bit[1]);

        seq.setBitFlipGate(0, false);
        seq.resetWriteHead(0);
        REQUIRE(seq.writeHead(0) == 1);
    }

    SECTION("le write head décalé boucle sur la longueur")
    {
        for (int i = 0; i < 7; ++i)
            seq.shiftWriteHead(0);
        REQUIRE(seq.writeHead(0) == 0); // 1 + 7 modulo 8
    }
}

TEST_CASE("Séquenceur : BIT FLIP maintenu inverse toute la séquence", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);
    const auto original = bitsOf(seq, 0);

    seq.tick(0);                 // pas 1
    seq.setBitFlipGate(0, true); // flippe le pas 1
    run(seq, 0, 7);              // puis les 7 suivants au passage
    seq.setBitFlipGate(0, false);

    for (std::size_t i = 0; i < 8; ++i)
        REQUIRE(seq.lane(0).bit[i] != original[i]);
}

TEST_CASE("Séquenceur : chain = une séquence de 16 pas", "[seq]")
{
    auto seq = makeSequencer();
    for (int i = 0; i < kMaxSteps; ++i)
    {
        seq.setStep(0, i, true, 0.1f * static_cast<float>(i + 1));
        seq.setStep(1, i, true, 1.0f + 0.1f * static_cast<float>(i + 1));
    }
    seq.setChain(true);
    REQUIRE(seq.span(0) == 16);
    REQUIRE(seq.span(1) == 16);

    SECTION("les deux lanes parcourent SEQ1 puis SEQ2, en phase")
    {
        for (int i = 0; i < 32; ++i)
        {
            seq.tick(0);
            seq.tick(1);
            const auto index = i % 16;
            const auto expected = (index < 8 ? 0.0f : 1.0f) + 0.1f * static_cast<float>(index % 8 + 1);
            REQUIRE(seq.cv(0) == Approx(expected));
            REQUIRE(seq.cv(1) == Approx(expected));
        }
    }

    SECTION("les longueurs s'additionnent")
    {
        seq.pressLength(0); // 7 + 8
        seq.pressLength(1);
        seq.pressLength(1); // 7 + 6
        REQUIRE(seq.span(0) == 13);
        const auto out = run(seq, 0, 13);
        REQUIRE(out[6].cv == Approx(0.7f));
        REQUIRE(out[7].cv == Approx(1.1f));
        REQUIRE(out[12].cv == Approx(1.6f));
        REQUIRE(run(seq, 0, 13) == out);
    }

    SECTION("BIT SHIFT 1 tourne les 16 pas ensemble")
    {
        seq.bitShift(0);
        REQUIRE(seq.lane(0).voltage[0] == Approx(1.8f)); // dernier pas de SEQ2
        REQUIRE(seq.lane(1).voltage[0] == Approx(0.8f)); // dernier pas de SEQ1
        seq.undoBitShift(0);
        REQUIRE(seq.lane(0).voltage[0] == Approx(0.1f));
        REQUIRE(seq.lane(1).voltage[0] == Approx(1.1f));
    }

    SECTION("BIT SHIFT 2 décale seulement le play head de SEQ2, puis resynchronisation")
    {
        const auto before = seq.lane(1).voltage;
        seq.bitShift(1);
        seq.bitShift(1);
        REQUIRE(seq.lane(1).voltage == before);
        REQUIRE(seq.lane(1).playHead == seq.lane(0).playHead + 2);

        seq.tick(0);
        seq.tick(1);
        REQUIRE(seq.cv(0) == Approx(0.1f));
        REQUIRE(seq.cv(1) == Approx(0.3f));

        seq.syncChainPlayHeads();
        REQUIRE(seq.lane(1).playHead == seq.lane(0).playHead);
    }

    SECTION("BIT FLIP 2 agit sur le pas partagé")
    {
        seq.tick(1); // play head de SEQ2 sur le pas 1 des 16, qui appartient à SEQ1
        seq.setBitFlipGate(1, true);
        REQUIRE_FALSE(seq.lane(0).bit[0]);
    }

    SECTION("sortie du mode chain : chaque lane retrouve sa boucle")
    {
        run(seq, 0, 12);
        run(seq, 1, 12);
        seq.setChain(false);
        REQUIRE(seq.span(0) == 8);
        REQUIRE(seq.lane(0).playHead < 8);
        REQUIRE(seq.lane(1).playHead < 8);
    }
}

TEST_CASE("Séquenceur : statistiques de CORRUPT", "[seq][stats]")
{
    constexpr int draws = 100000;
    auto seq = makeSequencer(2024);
    for (int i = 0; i < 7; ++i)
        seq.pressLength(0); // un seul pas : le write head repasse dessus à chaque tick

    SECTION("à 0,5 : pVolt ≈ 0,25 et aucun bit flip")
    {
        seq.setStep(0, 0, true, 0.0f);
        seq.setCorrupt(0, 0.5f);
        int changes = 0;
        for (int i = 0; i < draws; ++i)
        {
            const auto before = seq.lane(0).voltage[0];
            seq.tick(0);
            REQUIRE(seq.lane(0).bit[0]);
            changes += seq.lane(0).voltage[0] != before ? 1 : 0;
        }
        REQUIRE(static_cast<double>(changes) / draws == Approx(0.25).margin(0.005)); // ±2 %
    }

    SECTION("à 0,25 : pVolt ≈ 0,125")
    {
        seq.setStep(0, 0, true, 0.0f);
        seq.setCorrupt(0, 0.25f);
        int changes = 0;
        for (int i = 0; i < draws; ++i)
        {
            const auto before = seq.lane(0).voltage[0];
            seq.tick(0);
            changes += seq.lane(0).voltage[0] != before ? 1 : 0;
        }
        REQUIRE(static_cast<double>(changes) / draws == Approx(0.125).margin(0.0025));
    }

    SECTION("à 1,0 : pFlip ≈ 0,5, et un bit qui s'allume reçoit toujours une nouvelle tension")
    {
        seq.setStep(0, 0, true, 0.0f);
        seq.setCorrupt(0, 1.0f);
        int flips = 0;
        for (int i = 0; i < draws; ++i)
        {
            const auto bitBefore = seq.lane(0).bit[0];
            const auto voltBefore = seq.lane(0).voltage[0];
            seq.tick(0);
            const auto bitAfter = seq.lane(0).bit[0];
            flips += bitAfter != bitBefore ? 1 : 0;
            if (bitAfter && !bitBefore)
                REQUIRE(seq.lane(0).voltage[0] != voltBefore);
            if (!bitAfter)
                REQUIRE(seq.lane(0).voltage[0] == voltBefore); // éteint : tension gardée en mémoire
        }
        REQUIRE(static_cast<double>(flips) / draws == Approx(0.5).margin(0.01)); // ±2 %
    }

    SECTION("à 1,0 sans bit flip possible à observer : pVolt ≈ 0,5 sur les pas restés actifs")
    {
        seq.setStep(0, 0, true, 0.0f);
        seq.setCorrupt(0, 1.0f);
        int stayedOn = 0;
        int changes = 0;
        for (int i = 0; i < draws; ++i)
        {
            const auto bitBefore = seq.lane(0).bit[0];
            const auto voltBefore = seq.lane(0).voltage[0];
            seq.tick(0);
            if (bitBefore && seq.lane(0).bit[0])
            {
                ++stayedOn;
                changes += seq.lane(0).voltage[0] != voltBefore ? 1 : 0;
            }
        }
        REQUIRE(static_cast<double>(changes) / stayedOn == Approx(0.5).margin(0.01));
    }
}

TEST_CASE("Séquenceur : CORRUPT agit au write head, pas ailleurs", "[seq]")
{
    auto seq = makeSequencer();
    for (int i = 0; i < kMaxSteps; ++i)
        seq.setStep(0, i, true, 0.1f * static_cast<float>(i + 1));
    for (int i = 0; i < 6; ++i)
        seq.pressLength(0); // longueur 2 : les pas 3 à 8 ne sont jamais visités
    seq.setCorrupt(0, 1.0f);
    run(seq, 0, 2000);

    for (std::size_t i = 2; i < 8; ++i)
    {
        REQUIRE(seq.lane(0).bit[i]);
        REQUIRE(seq.lane(0).voltage[i] == 0.1f * static_cast<float>(i + 1));
    }
    REQUIRE(seq.lane(0).voltage[0] != 0.1f);
    // L'autre lane n'est pas touchée.
    REQUIRE(seq.lane(1).voltage == std::array<float, 8>{});
}

TEST_CASE("Séquenceur : même seed, même mutation", "[seq]")
{
    auto a = makeSequencer(99);
    auto b = makeSequencer(99);
    auto c = makeSequencer(100);
    for (auto* seq : {&a, &b, &c})
    {
        loadPattern(*seq);
        seq->setCorrupt(0, 0.9f);
    }
    const auto outA = run(a, 0, 500);
    REQUIRE(outA == run(b, 0, 500));
    REQUIRE(outA != run(c, 0, 500));
}

TEST_CASE("Séquenceur : CV RANGE, polarité et quantizer", "[seq]")
{
    auto seq = makeSequencer();
    seq.setStep(0, 0, true, 4.0f);
    seq.tick(0);

    REQUIRE(seq.cv(0) == Approx(4.0f));
    seq.setCvRange(0, 0.0f);
    REQUIRE(seq.cv(0) == Approx(0.4f)); // ±5 V ramenés à ±0,5 V
    seq.setCvRange(0, 0.5f);
    REQUIRE(seq.cv(0) == Approx(2.2f));

    seq.setCvRange(0, 1.0f);
    seq.setUnipolar(true);
    REQUIRE(seq.cv(0) == Approx(4.5f)); // 4 × 0,5 + 2,5
    seq.setUnipolar(false);

    seq.setStep(0, 0, true, 0.1f);
    seq.reset();
    seq.tick(0);
    seq.setQuantizerMode(2); // majeur : 1,2 demi-ton → ré
    REQUIRE(seq.cv(0) == Approx(2.0f / 12.0f));
    seq.setRootSemitones(1); // do dièse majeur : 1,2 demi-ton → do dièse
    REQUIRE(seq.cv(0) == Approx(1.0f / 12.0f));
}

TEST_CASE("Séquenceur : modes du quantizer", "[seq]")
{
    GenerativeSequencer seq(1);
    REQUIRE(seq.quantizerMode() == 2); // défaut : n°3, majeur
    seq.nextQuantizerMode();
    REQUIRE(seq.quantizerMode() == 3);
    seq.setQuantizerMode(15);
    seq.nextQuantizerMode();
    REQUIRE(seq.quantizerMode() == 0);
    seq.previousQuantizerMode();
    REQUIRE(seq.quantizerMode() == 15);
    seq.setQuantizerMode(42);
    REQUIRE(seq.quantizerMode() == 15);
}

TEST_CASE("Séquenceur : sauvegarde et rappel du buffer", "[seq]")
{
    auto seq = makeSequencer();
    loadPattern(seq);

    SECTION("rappel sans sauvegarde : sans effet")
    {
        const auto before = seq.lane(0);
        REQUIRE_FALSE(seq.hasBuffer());
        seq.recallBuffer();
        REQUIRE(seq.lane(0).bit == before.bit);
        REQUIRE(seq.lane(0).voltage == before.voltage);
    }

    SECTION("le rappel restaure bits, tensions, quantizer, offsets, longueurs et chain")
    {
        seq.pressLength(1);
        seq.shiftWriteHead(0);
        seq.setQuantizerMode(7);
        seq.setChain(true);
        const auto saved0 = seq.lane(0);
        const auto saved1 = seq.lane(1);
        seq.saveBuffer();
        REQUIRE(seq.hasBuffer());

        // On abîme tout.
        seq.setChain(false);
        seq.setQuantizerMode(1);
        seq.pressLength(0);
        seq.pressLength(1);
        seq.shiftWriteHead(0);
        seq.shiftWriteHead(1);
        seq.bitShift(0);
        seq.setCorrupt(0, 1.0f);
        seq.setCorrupt(1, 1.0f);
        run(seq, 0, 200);
        run(seq, 1, 200);

        seq.recallBuffer();
        for (int lane = 0; lane < 2; ++lane)
        {
            const auto& expected = lane == 0 ? saved0 : saved1;
            REQUIRE(seq.lane(lane).bit == expected.bit);
            REQUIRE(seq.lane(lane).voltage == expected.voltage);
            REQUIRE(seq.lane(lane).length == expected.length);
            REQUIRE(seq.lane(lane).writeOffset == expected.writeOffset);
            REQUIRE(seq.lane(lane).playHead < seq.span(lane));
        }
        REQUIRE(seq.quantizerMode() == 7);
        REQUIRE(seq.isChained());

        // Le slot reste disponible pour un second rappel.
        seq.setCorrupt(0, 1.0f);
        run(seq, 0, 200);
        seq.recallBuffer();
        REQUIRE(seq.lane(0).voltage == saved0.voltage);
    }

    SECTION("effacement : bits, quantizer et offsets")
    {
        seq.shiftWriteHead(0);
        seq.setQuantizerMode(9);
        seq.clear();
        for (int lane = 0; lane < 2; ++lane)
        {
            REQUIRE(seq.lane(lane).bit == std::array<bool, 8>{});
            REQUIRE(seq.lane(lane).writeOffset == 0);
        }
        REQUIRE(seq.quantizerMode() == 2);
        REQUIRE_FALSE(run(seq, 0, 8)[0].trigger);
    }
}
