#include "Engine.h"
#include "Patch/PatchBay.h"
#include "TestSignals.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace test;
using Catch::Approx;
using dedale::In;
using dedale::Out;

namespace
{
constexpr double baseRate = 48000.0;
constexpr int factor = 4;

// Moteur prêt à jouer, séquenceur en marche, avec un motif connu sur les deux lanes.
struct Rig
{
    Rig()
    {
        engine.prepare(baseRate, factor, 512);
        engine.sequencer().setRunning(true);
        engine.sequencer().setQuantizerMode(0);
        for (int i = 0; i < dedale::kMaxSteps; ++i)
        {
            engine.sequencer().setStep(0, i, i % 2 == 0, 0.0f);
            engine.sequencer().setStep(1, i, i % 3 == 0, 0.0f);
        }
    }

    // Fait tourner le moteur, avec un tick d'horloge maître tous les clockPeriod échantillons (0 = jamais).
    // onControl est appelé après chaque échantillon de contrôle. Renvoie l'audio suréchantillonné.
    template <typename Callback>
    std::vector<float> run(double seconds, int clockPeriod, Callback&& onControl, bool triggerAtStart = false)
    {
        std::vector<float> audio;
        const auto total = static_cast<int>(seconds * baseRate);
        audio.reserve(static_cast<std::size_t>(total * factor));
        for (int n = 0; n < total; ++n)
        {
            engine.processControl(params, clockPeriod > 0 && n % clockPeriod == 0, triggerAtStart && n == 0);
            onControl(n);
            for (int k = 0; k < factor; ++k)
                audio.push_back(engine.processAudio());
        }
        return audio;
    }

    std::vector<float> run(double seconds, int clockPeriod = 12000, bool triggerAtStart = false)
    {
        return run(seconds, clockPeriod, [](int) {}, triggerAtStart);
    }

    dedale::Engine engine{42};
    dedale::EngineParams params;
};

bool allFinite(const std::vector<float>& x)
{
    return std::all_of(x.begin(), x.end(), [](float v) { return std::isfinite(v); });
}
} // namespace

TEST_CASE("Patch bay : câbles et normalisations", "[patch]")
{
    dedale::PatchBay bay;
    bay.write(Out::eg1, 3.0f);
    bay.write(Out::mvco, -2.0f);

    SECTION("sans câble, l'entrée voit sa normalisation")
    {
        REQUIRE_FALSE(bay.isPatched(In::cutoff));
        REQUIRE(bay.source(In::cutoff) == -1);
        REQUIRE(bay.read(In::cutoff, 7.5f) == 7.5f);
    }

    SECTION("un câble casse la normalisation ; le retirer la rétablit")
    {
        REQUIRE(bay.connect(Out::mvco, In::cutoff));
        REQUIRE(bay.isPatched(In::cutoff));
        REQUIRE(bay.read(In::cutoff, 7.5f) == -2.0f);
        bay.write(Out::mvco, 1.25f);
        REQUIRE(bay.read(In::cutoff, 7.5f) == 1.25f);
        bay.disconnect(In::cutoff);
        REQUIRE(bay.read(In::cutoff, 7.5f) == 7.5f);
    }

    SECTION("une entrée n'accepte qu'un câble : le second remplace le premier")
    {
        bay.connect(Out::mvco, In::vcwFold);
        bay.connect(Out::eg1, In::vcwFold);
        REQUIRE(bay.source(In::vcwFold) == static_cast<int>(Out::eg1));
        REQUIRE(bay.read(In::vcwFold, 0.0f) == 3.0f);
    }

    SECTION("une sortie peut alimenter plusieurs entrées")
    {
        bay.connect(Out::eg1, In::vcwVcaCv);
        bay.connect(Out::eg1, In::blend);
        bay.connect(Out::eg1, In::bitFlip2);
        REQUIRE(bay.read(In::vcwVcaCv, 0.0f) == 3.0f);
        REQUIRE(bay.read(In::blend, 0.0f) == 3.0f);
        REQUIRE(bay.read(In::bitFlip2, 0.0f) == 3.0f);
    }

    SECTION("le jack MIDI n'est pas câblable ; clear retire tout")
    {
        REQUIRE_FALSE(bay.connect(Out::clock, In::midi));
        REQUIRE_FALSE(bay.isPatched(In::midi));
        bay.connect(Out::eg1, In::reset);
        bay.clear();
        for (int i = 0; i < dedale::kNumInputs; ++i)
            REQUIRE_FALSE(bay.isPatched(static_cast<In>(i)));
    }
}

TEST_CASE("Entrée gate : seuil à +1 V, hystérésis jusqu'à +0,5 V", "[patch]")
{
    dedale::GateDetector gate;
    REQUIRE_FALSE(gate.process(0.9f));
    REQUIRE(gate.process(1.1f));       // front montant
    REQUIRE_FALSE(gate.process(5.0f)); // déjà haut
    REQUIRE_FALSE(gate.process(0.7f)); // entre les deux seuils : reste haut
    REQUIRE(gate.isHigh());
    REQUIRE_FALSE(gate.process(1.2f)); // pas de nouveau front
    REQUIRE_FALSE(gate.process(0.4f));
    REQUIRE_FALSE(gate.isHigh());
    REQUIRE(gate.process(1.5f));
}

TEST_CASE("Moteur : sans câble, le patch init sonne", "[patch][engine]")
{
    Rig rig;
    const auto audio = rig.run(2.0);
    REQUIRE(allFinite(audio));
    REQUIRE(peak(audio) > 0.1f);
    REQUIRE(peak(audio) <= 1.0f);
}

TEST_CASE("Moteur : sorties du séquenceur et de l'horloge", "[patch][engine]")
{
    Rig rig;
    rig.engine.sequencer().setStep(0, 0, true, 2.0f);
    int trigHigh = 0;
    int clockHigh = 0;
    float cvSeen = 0.0f;
    rig.run(1.0, 12000,
            [&](int n)
            {
                const auto& bay = rig.engine.patchBay();
                trigHigh += bay.output(Out::seq1Trig) > 2.5f ? 1 : 0;
                clockHigh += bay.output(Out::clock) > 2.5f ? 1 : 0;
                if (n == 10)
                    cvSeen = bay.output(Out::seq1Cv);
            });
    // 4 ticks en 1 s ; SEQ1 a un pas actif sur deux : 2 triggers de 1 ms (48 échantillons), 4 impulsions
    // d'horloge.
    REQUIRE(trigHigh == 2 * 48);
    REQUIRE(clockHigh == 4 * 48);
    REQUIRE(cvSeen == Approx(2.0f));
}

TEST_CASE("Moteur : M VCO → BIT FLIP 1 flippe à l'audio-rate sans crash", "[patch][engine]")
{
    Rig rig;
    rig.params.voice.mvcoFreq = 500.0f;
    REQUIRE(rig.engine.patchBay().connect(Out::mvco, In::bitFlip1));

    auto previous = rig.engine.sequencer().lane(0).bit;
    int flips = 0;
    const auto audio = rig.run(1.0, 12000,
                               [&](int)
                               {
                                   const auto& bits = rig.engine.sequencer().lane(0).bit;
                                   flips += bits != previous ? 1 : 0;
                                   previous = bits;
                               });
    REQUIRE(allFinite(audio));
    REQUIRE(flips > 450); // un flip par front montant du triangle (500 par seconde), plus ceux des ticks
    REQUIRE(flips < 560);
    // L'autre lane n'est pas touchée.
    for (int i = 0; i < dedale::kMaxSteps; ++i)
        REQUIRE(rig.engine.sequencer().lane(1).bit[static_cast<std::size_t>(i)] == (i % 3 == 0));
}

TEST_CASE("Moteur : CLOCK 2 indépendant donne de la polymétrie", "[patch][engine]")
{
    Rig rig;
    auto countSteps = [&](double seconds)
    {
        std::array<int, 2> steps{};
        std::array<int, 2> heads{rig.engine.sequencer().lane(0).playHead,
                                 rig.engine.sequencer().lane(1).playHead};
        rig.run(seconds, 12000,
                [&](int)
                {
                    for (std::size_t lane = 0; lane < 2; ++lane)
                    {
                        const auto head = rig.engine.sequencer().lane(static_cast<int>(lane)).playHead;
                        steps[lane] += head != heads[lane] ? 1 : 0;
                        heads[lane] = head;
                    }
                });
        return steps;
    };

    SECTION("sans câble : CLOCK 2 suit CLOCK 1, les deux lanes avancent ensemble")
    {
        const auto steps = countSteps(10.0);
        REQUIRE(steps[0] == 39); // 40 ticks, le premier joue le pas 1 sans avancer
        REQUIRE(steps[1] == 39);
    }

    SECTION("M VCO lent dans CLOCK 2 : SEQ2 a son propre tempo")
    {
        rig.params.voice.mvcoFreq = 3.0f;
        rig.engine.patchBay().connect(Out::mvco, In::clock2);
        const auto steps = countSteps(10.0);
        REQUIRE(steps[0] == 39); // horloge maître : 4 pas par seconde
        REQUIRE(steps[1] >= 28); // M VCO : 3 pas par seconde
        REQUIRE(steps[1] <= 30);
    }

    SECTION("un câble dans CLOCK 1 entraîne aussi SEQ2 (CLOCK 2 est normalisé sur CLOCK 1)")
    {
        rig.params.voice.mvcoFreq = 3.0f;
        rig.engine.patchBay().connect(Out::mvco, In::clock1);
        const auto steps = countSteps(10.0);
        REQUIRE(steps[0] >= 28);
        REQUIRE(steps[0] <= 30);
        REQUIRE(steps[1] == steps[0]);
    }
}

TEST_CASE("Moteur : RESET ramène au pas 1", "[patch][engine]")
{
    Rig rig;
    rig.engine.sequencer().shiftWriteHead(0);
    rig.engine.patchBay().connect(Out::clock, In::reset); // un reset à chaque tick d'horloge
    int farthest = 0;
    rig.run(4.0, 12000, [&](int) { farthest = std::max(farthest, rig.engine.sequencer().lane(0).playHead); });
    REQUIRE(farthest <= 1); // le séquenceur n'a jamais le temps d'aller plus loin
    REQUIRE(rig.engine.sequencer().lane(0).writeOffset == 1); // offset du write head conservé
}

TEST_CASE("Moteur : TRIGGER et EG2 TRIG", "[patch][engine]")
{
    Rig rig;
    rig.engine.sequencer().setRunning(false);
    auto peaks = [&](bool manual, int clockPeriod = 0)
    {
        std::array<float, 2> top{};
        rig.run(
            0.05, clockPeriod,
            [&](int)
            {
                top[0] = std::max(top[0], rig.engine.patchBay().output(Out::eg1));
                top[1] = std::max(top[1], rig.engine.patchBay().output(Out::eg2));
            },
            manual);
        return top;
    };

    SECTION("sans câble, un trigger manuel déclenche les deux enveloppes à pleine vélocité")
    {
        const auto top = peaks(true);
        REQUIRE(top[0] == Approx(8.0f).epsilon(0.05));
        REQUIRE(top[1] == Approx(8.0f).epsilon(0.05));
    }

    SECTION("rien ne se déclenche tout seul à l'arrêt")
    {
        const auto top = peaks(false, 480);
        REQUIRE(top[0] == 0.0f);
        REQUIRE(top[1] == 0.0f);
    }

    SECTION("le jack TRIGGER déclenche les deux enveloppes")
    {
        rig.engine.patchBay().connect(Out::clock, In::trigger);
        const auto top = peaks(false, 480);
        REQUIRE(top[0] > 7.0f);
        REQUIRE(top[1] > 7.0f);
    }

    SECTION("EG2 TRIG patché : TRIGGER ne déclenche plus que EG1")
    {
        rig.engine.patchBay().connect(Out::seq2Trig, In::eg2Trig); // SEQ2 à l'arrêt : jamais de front
        const auto top = peaks(true);
        REQUIRE(top[0] > 7.0f);
        REQUIRE(top[1] == 0.0f);
    }

    SECTION("EG2 TRIG déclenche EG2 seul")
    {
        rig.engine.patchBay().connect(Out::clock, In::eg2Trig);
        const auto top = peaks(false, 480);
        REQUIRE(top[0] == 0.0f);
        REQUIRE(top[1] > 7.0f);
    }
}

TEST_CASE("Moteur : boucles de feedback stables", "[patch][engine]")
{
    Rig rig;
    auto& v = rig.params.voice;
    v.mixVco = v.mixRing = v.mixMvco = v.mixNoise = 1.0f;
    v.fold = v.resonance = v.volume = v.fmAmt = 1.0f;
    v.umix1Level = 1.0f;
    rig.params.eg2Decay = 10.0f;
    auto& bay = rig.engine.patchBay();
    bay.connect(Out::vca, In::vcfIn);    // la sortie revient dans le filtre...
    bay.connect(Out::vca, In::vcwIn);    // ... et dans le folder
    bay.connect(Out::vca, In::vcoPitch); // et module la hauteur du VCO
    bay.connect(Out::umix, In::cutoff);
    bay.connect(Out::umix, In::umix2); // boucle directe d'une sortie sur son propre mixeur
    bay.connect(Out::mixer, In::blend);
    bay.connect(Out::noise, In::mvcoPitch);
    bay.connect(Out::mvco, In::vcwFold);
    bay.connect(Out::vca, In::vcwVcaCv);

    const auto audio = rig.run(2.0);
    REQUIRE(allFinite(audio));
    REQUIRE(peak(audio) <= 1.0f);
    REQUIRE(std::isfinite(bay.output(Out::umix)));
    REQUIRE(std::abs(bay.output(Out::umix)) <= 50.0f);
}

TEST_CASE("Voix : entrées de la patch bay", "[patch][voice]")
{
    dedale::Voice voice;
    dedale::PatchBay bay;
    voice.prepare(sampleRate, 512);
    dedale::VoiceParams params;
    params.fold = 0.0f;
    params.vcwEg1Amt = 0.0f;
    params.vcfEg1Amt = 0.0f;
    dedale::VoiceCv cv{0.0f, 0.0f, 0.0f, 8.0f}; // VCA ouverts

    auto render = [&](int samples = 96000)
    {
        voice.reset();
        std::vector<float> out(static_cast<std::size_t>(samples));
        for (auto& v : out)
        {
            voice.setControls(params, cv, bay); // comme le moteur : une fois par échantillon de base
            v = voice.processSample(bay);
        }
        return out;
    };

    SECTION("CUTOFF patché : EG1 n'agit plus sur le filtre")
    {
        params.blend = 1.0f;
        params.cutoff = 60.0f; // sous la fondamentale : fermé, le filtre ne laisse presque que 110 Hz
        params.mixVco = 1.0f;  // mixeur saturé : riche en harmoniques
        params.vcfEg1Amt = 1.0f;
        cv.eg1 = 8.0f;
        const auto open = distortion(render(), 110.0); // EG1 ouvre le filtre de 8 octaves
        bay.connect(Out::eg2, In::cutoff);             // sortie EG2 à 0 V dans cette patch bay de test
        const auto closed = distortion(render(), 110.0);
        REQUIRE(open > 3.0 * closed);

        bay.write(Out::eg2, 5.0f); // +5 V dans CUTOFF avec l'atténuverteur à fond : 5 octaves
        REQUIRE(distortion(render(), 110.0) > 2.0 * closed);
    }

    SECTION("VCW VCA CV patché : le chemin VCW ne suit plus EG2")
    {
        cv.eg2 = 0.0f; // EG2 fermé : silence sans câble
        REQUIRE(peak(render()) < 1e-5f);
        bay.write(Out::eg1, 8.0f);
        bay.connect(Out::eg1, In::vcwVcaCv);
        params.blend = 0.0f;
        REQUIRE(peak(render()) > 0.1f); // le chemin VCW s'ouvre
        params.blend = 1.0f;
        REQUIRE(peak(render()) < 1e-5f); // le chemin VCF reste fermé
    }

    SECTION("VCW IN remplace la source du folder")
    {
        params.blend = 0.0f;
        const auto normal = componentAmplitude(render(), 110.0, sampleRate);
        bay.connect(Out::noise, In::vcwIn);
        const auto out = render();
        REQUIRE(peak(out) > 0.05f);
        REQUIRE(componentAmplitude(out, 110.0, sampleRate) < 0.1 * normal);
    }

    SECTION("VCF IN remplace la source du filtre, même en série")
    {
        params.blend = 1.0f;
        params.order = dedale::Order::vcwToVcf;
        params.cutoff = 5000.0f;
        bay.connect(Out::mvco, In::vcfIn);
        params.mvcoFreq = 300.0f;
        const auto out = render();
        REQUIRE(componentAmplitude(out, 300.0, sampleRate) >
                10.0 * componentAmplitude(out, 110.0, sampleRate));
    }

    SECTION("VCO 1V/OCT : +1 V = une octave, et M VCO dedans = FM à l'audio-rate")
    {
        params.blend = 0.0f;
        bay.write(Out::eg1, 1.0f);
        bay.connect(Out::eg1, In::vcoPitch);
        REQUIRE(measureFrequency(render(), sampleRate) == Approx(220.0).epsilon(0.005));

        const auto pure = distortion(render(), 220.0);
        bay.connect(Out::mvco, In::vcoPitch);
        params.mvcoFreq = 150.0f;
        REQUIRE(distortion(render(), 110.0) > 10.0 * pure);
    }

    SECTION("M VCO 1V/OCT et SYNC")
    {
        params.mixVco = 0.0f;
        params.mixMvco = 0.5f;
        params.blend = 0.0f;
        params.mvcoFreq = 100.0f;
        bay.write(Out::eg1, 1.0f);
        bay.connect(Out::eg1, In::mvcoPitch);
        REQUIRE(measureFrequency(render(), sampleRate) == Approx(200.0).epsilon(0.01));

        bay.write(Out::eg2, 5.0f); // gate haut : phase bloquée, le triangle ne bouge plus
        bay.connect(Out::eg2, In::mvcoSync);
        render();
        REQUIRE(bay.output(Out::mvco) == Approx(5.0f).margin(0.05f));
        const auto held = render(48000);
        REQUIRE(peak(std::vector<float>(held.begin() + 40000, held.end())) < 0.01f); // DC bloqué : silence
    }

    SECTION("BLEND : le jack s'ajoute au knob")
    {
        params.cutoff = 200.0f;
        params.fold = 0.8f;
        params.blend = 0.0f; // knob sur VCW : brillant
        const auto bright = distortion(render(), 110.0);
        bay.write(Out::eg1, 5.0f); // +5 V : toute la course, donc VCF
        bay.connect(Out::eg1, In::blend);
        REQUIRE(distortion(render(), 110.0) < 0.25 * bright);
    }

    SECTION("U MIX : canal 1 normalisé sur le ring mod, canal 2 à gain unité")
    {
        params.umix1Level = 0.0f;
        render(4800);
        REQUIRE(bay.output(Out::umix) == 0.0f);

        bay.write(Out::eg1, 2.5f);
        bay.connect(Out::eg1, In::umix2);
        render(4800);
        REQUIRE(bay.output(Out::umix) == Approx(2.5f)); // DC-coupled, gain unité

        bay.disconnect(In::umix2);
        params.umix1Level = 1.0f;
        float top = 0.0f;
        voice.reset();
        for (int i = 0; i < 96000; ++i)
        {
            voice.setControls(params, cv, bay);
            voice.processSample(bay);
            top = std::max(top, std::abs(bay.output(Out::umix)));
        }
        REQUIRE(top > 2.0f); // le ring mod (VCO × MOD VCO) passe
        REQUIRE(top <= 5.01f);

        bay.connect(Out::eg1, In::umix1); // patcher U MIX 1 casse la normalisation sur le ring mod
        params.umix1Level = 0.5f;
        render(4800);
        REQUIRE(bay.output(Out::umix) == Approx(1.25f));
    }

    SECTION("sorties M VCO, NOISE, MIXER et VCA alimentées")
    {
        params.mixNoise = 0.3f;
        std::array<float, 4> top{};
        voice.reset();
        for (int i = 0; i < 96000; ++i)
        {
            voice.setControls(params, cv, bay);
            const auto out = voice.processSample(bay);
            top[0] = std::max(top[0], std::abs(bay.output(Out::mvco)));
            top[1] = std::max(top[1], std::abs(bay.output(Out::noise)));
            top[2] = std::max(top[2], std::abs(bay.output(Out::mixer)));
            top[3] = std::max(top[3], std::abs(bay.output(Out::vca)));
            REQUIRE(bay.output(Out::vca) == Approx(out * 5.0f).margin(1e-4)); // jack VCA en volts
        }
        REQUIRE(top[0] == Approx(5.0f).margin(0.1f));
        REQUIRE(top[1] > 1.0f);
        REQUIRE(top[2] > 3.0f);
        REQUIRE(top[3] > 1.0f);
    }
}
