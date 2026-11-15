#include "Calibration.h"
#include "DSP/DecayEnv.h"
#include "DSP/ModVco.h"
#include "DSP/Noise.h"
#include "DSP/SaturatingMixer.h"
#include "DSP/VcFilter.h"
#include "DSP/Vco.h"
#include "DSP/Voice.h"
#include "DSP/Wavefolder.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>
#include <vector>

using Catch::Approx;
namespace cal = dedale::calibration;

namespace
{
constexpr double sampleRate = 192000.0; // la voix tourne suréchantillonnée ×4
constexpr float pi = std::numbers::pi_v<float>;

// Fréquence mesurée par les passages à zéro montants, avec interpolation linéaire.
double measureFrequency(const std::vector<float>& x, double rate)
{
    double first = -1.0;
    double last = -1.0;
    int count = 0;
    for (std::size_t i = 1; i < x.size(); ++i)
    {
        if (x[i - 1] < 0.0f && x[i] >= 0.0f)
        {
            const auto t = static_cast<double>(i - 1) + static_cast<double>(-x[i - 1] / (x[i] - x[i - 1]));
            if (first < 0.0)
                first = t;
            last = t;
            ++count;
        }
    }
    return count > 1 ? (count - 1) * rate / (last - first) : 0.0;
}

double rms(const std::vector<float>& x, std::size_t from = 0)
{
    double sum = 0.0;
    for (std::size_t i = from; i < x.size(); ++i)
        sum += static_cast<double>(x[i]) * x[i];
    return std::sqrt(sum / static_cast<double>(x.size() - from));
}

float peak(const std::vector<float>& x)
{
    float p = 0.0f;
    for (const auto v : x)
        p = std::max(p, std::abs(v));
    return p;
}

// Amplitude de la composante à la fréquence donnée (corrélation avec sinus et cosinus).
double componentAmplitude(const std::vector<float>& x, double frequency, double rate)
{
    double re = 0.0;
    double im = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i)
    {
        const auto phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / rate;
        re += x[i] * std::cos(phase);
        im += x[i] * std::sin(phase);
    }
    return 2.0 * std::sqrt(re * re + im * im) / static_cast<double>(x.size());
}

std::vector<float> sine(double frequency, float amplitude, int samples)
{
    std::vector<float> x(static_cast<std::size_t>(samples));
    for (std::size_t i = 0; i < x.size(); ++i)
        x[i] = amplitude * static_cast<float>(std::sin(2.0 * std::numbers::pi * frequency *
                                                       static_cast<double>(i) / sampleRate));
    return x;
}

// Taux de distorsion : tout ce qui n'est pas la fondamentale, rapporté à la fondamentale.
double distortion(const std::vector<float>& x, double frequency)
{
    const auto fundamental = componentAmplitude(x, frequency, sampleRate) / std::sqrt(2.0);
    const auto total = rms(x);
    return std::sqrt(std::max(0.0, total * total - fundamental * fundamental)) / fundamental;
}
} // namespace

TEST_CASE("VCO : fréquence juste et 1 V/octave sur 5 octaves", "[dsp][vco]")
{
    dedale::Vco vco;
    vco.prepare(sampleRate, 512);
    for (int octave = 0; octave <= 5; ++octave)
    {
        const auto target = 110.0 * std::pow(2.0, octave);
        vco.reset();
        std::vector<float> out(static_cast<std::size_t>(sampleRate));
        for (auto& v : out)
            v = vco.processSample(static_cast<float>(target), 0.0f, 0.0f);
        const auto cents = 1200.0 * std::log2(measureFrequency(out, sampleRate) / target);
        INFO("octave " << octave << " : " << cents << " cents");
        REQUIRE(std::abs(cents) < 3.0);
        REQUIRE(peak(out) == Approx(5.0f).margin(0.01f)); // ±5 V
    }
}

TEST_CASE("VCO : TZFM through-zero", "[dsp][vco]")
{
    dedale::Vco vco;
    vco.prepare(sampleRate, 512);

    SECTION("une fréquence instantanée négative fait reculer la phase : le sinus est joué à l'envers")
    {
        std::vector<float> forward(2000);
        std::vector<float> backward(2000);
        for (auto& v : forward)
            v = vco.processSample(440.0f, 0.0f, 0.0f);
        vco.reset();
        for (auto& v : backward)
            v = vco.processSample(440.0f, -1.0f, 2.0f); // f0 × (1 + 2 × −1) = −f0
        for (std::size_t i = 0; i < forward.size(); ++i)
            REQUIRE(backward[i] == Approx(-forward[i]).margin(0.01f));
    }

    SECTION("un modulateur symétrique laisse la hauteur centrée et la sortie bornée")
    {
        std::vector<float> out(static_cast<std::size_t>(sampleRate));
        for (std::size_t i = 0; i < out.size(); ++i)
        {
            const auto mod =
                std::sin(2.0f * pi * 220.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
            out[i] = vco.processSample(220.0f, mod, cal::kTzfmMaxIndex);
        }
        REQUIRE(peak(out) <= 5.001f);
        // Rapport 1:1 : le spectre reste harmonique de 220 Hz, la composante à 220 Hz ne disparaît pas
        // ailleurs.
        REQUIRE(componentAmplitude(out, 220.0, sampleRate) + componentAmplitude(out, 440.0, sampleRate) >
                0.5);
        REQUIRE(componentAmplitude(out, 330.0, sampleRate) < 0.05);
    }
}

TEST_CASE("VCO SEQ AMT : cran QTZ en butée", "[dsp][vco]")
{
    REQUIRE(dedale::sequencerAmountGain(1.0f) == 1.0f);
    REQUIRE(dedale::sequencerAmountGain(0.98f) == 1.0f); // derniers 3 % : gain verrouillé
    REQUIRE(dedale::sequencerAmountGain(0.0f) == 0.0f);
    REQUIRE(dedale::sequencerAmountGain(0.5f) < 0.55f);
    REQUIRE(dedale::sequencerAmountGain(0.969f) ==
            Approx(1.0f).margin(0.01f)); // pas de saut à l'entrée du cran
}

TEST_CASE("MOD VCO : triangle juste, borné et anti-aliasé", "[dsp][mvco]")
{
    dedale::ModVco mvco;
    mvco.prepare(48000.0, 512);

    SECTION("fréquence et amplitude")
    {
        for (double target : {0.5, 55.0, 1300.0})
        {
            mvco.reset();
            std::vector<float> out(48000 * 8);
            for (auto& v : out)
                v = mvco.processSample(static_cast<float>(target), false);
            REQUIRE(measureFrequency(out, 48000.0) == Approx(target).epsilon(0.005));
            REQUIRE(peak(out) <= 5.05f);
            REQUIRE(peak(out) > 4.7f);
            REQUIRE(std::abs(rms(out) - 5.0 / std::sqrt(3.0)) < 0.1); // RMS d'un triangle ±5 V
        }
    }

    SECTION("moins de repliement que le triangle naïf")
    {
        // À 1300 Hz et 48 kHz, les harmoniques 19, 21 et 23 se replient à 23,3 / 20,7 / 18,1 kHz,
        // entre deux harmoniques : on y mesure l'énergie repliée.
        const auto frequency = 1300.0;
        std::vector<float> blamp(48000);
        std::vector<float> naive(48000);
        mvco.reset();
        for (std::size_t n = 0; n < blamp.size(); ++n)
        {
            const auto phase = std::fmod(static_cast<double>(n + 1) * frequency / 48000.0, 1.0);
            naive[n] = static_cast<float>(5.0 * (4.0 * std::abs(phase - 0.5) - 1.0));
            blamp[n] = mvco.processSample(static_cast<float>(frequency), false);
        }
        auto aliases = [](const std::vector<float>& x)
        {
            return componentAmplitude(x, 23300.0, 48000.0) + componentAmplitude(x, 20700.0, 48000.0) +
                   componentAmplitude(x, 18100.0, 48000.0);
        };
        REQUIRE(aliases(naive) > 1e-3);
        REQUIRE(aliases(blamp) < 0.5 * aliases(naive));
    }

    SECTION("SYNC : phase remise à zéro tant que le gate est haut")
    {
        for (int i = 0; i < 1000; ++i)
            mvco.processSample(100.0f, false);
        const auto held = mvco.processSample(100.0f, true);
        for (int i = 0; i < 100; ++i)
            REQUIRE(mvco.processSample(100.0f, true) == held);
    }
}

TEST_CASE("NOISE : niveau stable, tonalité du grave à l'aigu", "[dsp][noise]")
{
    dedale::Noise noise;
    noise.prepare(sampleRate, 512);

    auto render = [&](float tone)
    {
        noise.reset();
        noise.setTone(tone);
        std::vector<float> out(static_cast<std::size_t>(sampleRate) * 2);
        for (auto& v : out)
            v = noise.processSample();
        return out;
    };
    // Nombre de passages à zéro : monte avec la fréquence centrale.
    auto crossings = [](const std::vector<float>& x)
    {
        int count = 0;
        for (std::size_t i = 1; i < x.size(); ++i)
            count += (x[i - 1] < 0.0f) != (x[i] < 0.0f) ? 1 : 0;
        return count;
    };

    const auto dark = render(0.0f);
    const auto mid = render(0.5f);
    const auto bright = render(1.0f);
    REQUIRE(crossings(dark) < crossings(mid));
    REQUIRE(crossings(mid) < crossings(bright));
    for (const auto* out : {&dark, &mid, &bright})
    {
        REQUIRE(rms(*out, 20000) > 0.5 * cal::kNoiseRmsVolts);
        REQUIRE(rms(*out, 20000) < 2.0 * cal::kNoiseRmsVolts);
        REQUIRE(peak(*out) < 12.0f);
    }
}

TEST_CASE("MIXER : gain unité à midi, saturation au-delà", "[dsp][mixer]")
{
    dedale::SaturatingMixer mixer;
    mixer.prepare(sampleRate, 512);

    auto run = [&](float level, float amplitude)
    {
        mixer.reset();
        mixer.setLevels(level, 0.0f, 0.0f, 0.0f);
        auto x = sine(220.0, amplitude, 96000);
        for (auto& v : x)
            v = mixer.processSample(v, 0.0f, 0.0f, 0.0f);
        x.erase(x.begin(), x.begin() + 48000); // laisse le DC-block se poser
        return x;
    };

    SECTION("à midi, un petit signal passe à gain unité")
    {
        const auto out = run(0.5f, 0.5f);
        REQUIRE(componentAmplitude(out, 220.0, sampleRate) == Approx(0.5).epsilon(0.03));
    }
    SECTION("à zéro, silence")
    {
        REQUIRE(peak(run(0.0f, 5.0f)) < 1e-4f);
    }
    SECTION("à midi, ±5 V passent presque propres ; à fond, ça sature fort mais reste borné")
    {
        const auto clean = run(0.5f, 5.0f);
        const auto driven = run(1.0f, 5.0f);
        REQUIRE(distortion(clean, 220.0) < 0.08);
        REQUIRE(distortion(driven, 220.0) > 0.25);
        REQUIRE(peak(driven) < 5.0f * cal::kMixerSumCeiling + 0.5f);
        REQUIRE(peak(driven) > peak(clean));
    }
    SECTION("asymétrie : des harmoniques paires apparaissent, sans composante continue")
    {
        const auto driven = run(0.9f, 5.0f);
        REQUIRE(componentAmplitude(driven, 440.0, sampleRate) > 0.02);
        double mean = 0.0;
        for (const auto v : driven)
            mean += v;
        REQUIRE(std::abs(mean / static_cast<double>(driven.size())) < 0.02);
    }
}

TEST_CASE("VCW : propre à zéro, plié à fond, borné, sans composante continue", "[dsp][vcw]")
{
    dedale::Wavefolder folder;
    folder.prepare(sampleRate, 512);

    auto run = [&](float fold, float bias)
    {
        folder.reset();
        auto x = sine(110.0, 5.0f, 192000);
        for (auto& v : x)
            v = folder.processSample(v, fold, bias);
        x.erase(x.begin(), x.begin() + 96000);
        return x;
    };

    const auto clean = run(0.0f, 0.0f);
    const auto folded = run(1.0f, 0.0f);
    REQUIRE(distortion(clean, 110.0) < 0.08); // coloration résiduelle seulement
    REQUIRE(peak(clean) > 3.0f);
    REQUIRE(distortion(folded, 110.0) > 1.0); // la fondamentale n'est plus dominante
    REQUIRE(peak(folded) <= 5.5f);

    // Dès les premiers réglages de FOLD, les harmoniques dominent (pas de progression monotone : selon le
    // réglage, les plis renforcent ou annulent la fondamentale).
    REQUIRE(distortion(run(0.2f, 0.0f), 110.0) > 0.5);
    REQUIRE(distortion(run(0.5f, 0.0f), 110.0) > 0.5);

    // BIAS déplace la symétrie : harmoniques paires, mais pas de DC en sortie.
    const auto biased = run(0.5f, 0.6f);
    const auto centred = run(0.5f, 0.0f);
    REQUIRE(componentAmplitude(biased, 220.0, sampleRate) >
            4.0 * componentAmplitude(centred, 220.0, sampleRate));
    double mean = 0.0;
    for (const auto v : biased)
        mean += v;
    REQUIRE(std::abs(mean / static_cast<double>(biased.size())) < 0.05);
}

TEST_CASE("VCF : −3 dB au cutoff, pas de perte de graves, pas d'auto-oscillation", "[dsp][vcf]")
{
    dedale::VcFilter filter;
    filter.prepare(sampleRate, 512);

    auto gainDb = [&](double frequency, float cutoff, float resonance, float mode)
    {
        filter.reset();
        filter.setCutoff(cutoff, resonance);
        auto x = sine(frequency, 0.1f, 96000);
        for (auto& v : x)
            v = filter.processSample(v, mode);
        x.erase(x.begin(), x.begin() + 48000);
        return 20.0 * std::log10(componentAmplitude(x, frequency, sampleRate) / 0.1);
    };

    SECTION("passe-bas : −3 dB au cutoff, 12 dB par octave au-dessus")
    {
        for (float cutoff : {100.0f, 800.0f, 5000.0f})
        {
            REQUIRE(gainDb(cutoff, cutoff, 0.0f, 0.0f) == Approx(-3.01).margin(0.5));
            REQUIRE(gainDb(cutoff * 0.9, cutoff, 0.0f, 0.0f) > -3.01);
            REQUIRE(gainDb(cutoff * 1.1, cutoff, 0.0f, 0.0f) < -3.01);
            REQUIRE(gainDb(cutoff * 4.0, cutoff, 0.0f, 0.0f) == Approx(-24.0).margin(1.5));
        }
    }

    SECTION("la résonance ne mange pas les graves")
    {
        REQUIRE(gainDb(50.0, 1000.0f, 0.0f, 0.0f) == Approx(0.0).margin(0.3));
        REQUIRE(gainDb(50.0, 1000.0f, 1.0f, 0.0f) == Approx(0.0).margin(0.3));
        REQUIRE(gainDb(1000.0, 1000.0f, 1.0f, 0.0f) > 12.0); // pic de résonance
    }

    SECTION("passe-bande : centré sur le cutoff, niveau compensé")
    {
        REQUIRE(gainDb(1000.0, 1000.0f, 0.0f, 1.0f) == Approx(0.0).margin(0.5));
        REQUIRE(gainDb(100.0, 1000.0f, 0.0f, 1.0f) < -15.0);
        REQUIRE(gainDb(10000.0, 1000.0f, 0.0f, 1.0f) < -15.0);
        // Morph : à mi-course, entre les deux réponses.
        const auto mid = gainDb(100.0, 1000.0f, 0.0f, 0.5f);
        REQUIRE(mid < -3.0);
        REQUIRE(mid > -15.0);
    }

    SECTION("résonance max : sans entrée, la sortie retombe au silence")
    {
        filter.reset();
        filter.setCutoff(1000.0f, 1.0f);
        filter.processSample(5.0f, 0.0f); // impulsion
        float tail = 0.0f;
        for (int i = 0; i < 192000; ++i)
        {
            const auto out = filter.processSample(0.0f, 0.0f);
            if (i > 96000)
                tail = std::max(tail, std::abs(out));
        }
        REQUIRE(tail < 1e-4f);
    }

    SECTION("stable quand on pousse fort et qu'on module le cutoff à chaque échantillon")
    {
        filter.reset();
        float worst = 0.0f;
        for (int i = 0; i < 192000; ++i)
        {
            const auto phase = static_cast<float>(i) / static_cast<float>(sampleRate);
            filter.setCutoff(2000.0f * std::exp2(3.0f * std::sin(2.0f * pi * 700.0f * phase)), 1.0f);
            const auto out = filter.processSample(10.0f * std::sin(2.0f * pi * 150.0f * phase), 0.3f);
            REQUIRE(std::isfinite(out));
            worst = std::max(worst, std::abs(out));
        }
        REQUIRE(worst < 60.0f);
    }
}

TEST_CASE("VCA : 0 V = silence total, réponse croissante", "[dsp][vca]")
{
    REQUIRE(dedale::vcaGain(0.0f) == 0.0f);
    REQUIRE(dedale::vcaGain(-1.0f) == 0.0f);
    REQUIRE(dedale::vcaGain(8.0f) == Approx(1.0f));
    REQUIRE(dedale::vcaGain(12.0f) == Approx(1.0f));
    REQUIRE(dedale::vcaGain(4.0f) < 0.5f); // légèrement exponentielle
    for (float v = 0.5f; v <= 8.0f; v += 0.5f)
        REQUIRE(dedale::vcaGain(v) > dedale::vcaGain(v - 0.5f));
}

TEST_CASE("Enveloppe : decay à −60 dB, crête selon la vélocité, pas de saut au retrigger", "[dsp][eg]")
{
    constexpr double rate = 48000.0;
    dedale::DecayEnv env;
    env.prepare(rate, 512);

    SECTION("temps de decay à −60 dB")
    {
        for (float decay : {0.01f, 0.3f, 2.0f})
        {
            env.reset();
            env.trigger(1.0f);
            int samples = 0;
            float top = 0.0f;
            for (; samples < 48000 * 12; ++samples)
            {
                const auto out = env.processSample(decay);
                top = std::max(top, out);
                if (samples > 100 && out < 0.008f)
                    break;
            }
            INFO("decay " << decay);
            REQUIRE(top == Approx(8.0f).epsilon(0.1)); // 0 → +8 V
            REQUIRE(samples / rate == Approx(decay).epsilon(0.1).margin(0.002));
        }
    }

    SECTION("au repos : 0 V exactement")
    {
        env.reset();
        REQUIRE(env.processSample(0.3f) == 0.0f);
    }

    SECTION("la vélocité fixe la crête ; vélocité nulle = pas de trigger")
    {
        env.reset();
        env.trigger(0.5f);
        float top = 0.0f;
        for (int i = 0; i < 2000; ++i)
            top = std::max(top, env.processSample(2.0f));
        REQUIRE(top == Approx(4.0f).epsilon(0.05));

        env.reset();
        env.trigger(0.0f);
        REQUIRE(env.processSample(2.0f) == 0.0f);
    }

    SECTION("retrigger : la sortie ne saute pas d'un échantillon à l'autre")
    {
        env.reset();
        float previous = 0.0f;
        float worstStep = 0.0f;
        for (int i = 0; i < 48000; ++i)
        {
            if (i % 3000 == 0)
                env.trigger(i % 6000 == 0 ? 1.0f : 0.3f); // vélocités alternées
            const auto out = env.processSample(0.5f);
            worstStep = std::max(worstStep, std::abs(out - previous));
            previous = out;
        }
        REQUIRE(worstStep < 0.5f); // 8 V en moins d'une milliseconde, mais jamais d'un coup
    }
}

TEST_CASE("Voix : le patch init sonne sans câble, et se tait sans enveloppe", "[dsp][voice]")
{
    dedale::Voice voice;
    voice.prepare(sampleRate, 512);
    dedale::VoiceParams params; // valeurs par défaut du §10

    auto render = [&](dedale::VoiceCv cv)
    {
        voice.reset();
        std::vector<float> out(96000);
        voice.setControls(params, cv);
        for (auto& v : out)
            v = voice.processSample();
        return out;
    };

    SECTION("VCA fermés : silence")
    {
        REQUIRE(peak(render({0.0f, 0.0f, 0.0f, 0.0f})) < 1e-5f);
    }

    SECTION("VCA ouverts : du son, dans ±1, à la bonne hauteur")
    {
        const auto out = render({0.0f, 0.0f, 0.0f, 8.0f});
        REQUIRE(peak(out) > 0.1f);
        REQUIRE(peak(out) <= 1.0f);
        REQUIRE(componentAmplitude(out, 110.0, sampleRate) > 0.05);
    }

    SECTION("SEQ1 à +1 V avec le cran QTZ : une octave au-dessus")
    {
        params.fold = 0.0f;
        params.vcwEg1Amt = 0.0f;
        params.blend = 0.0f; // chemin VCW seul
        const auto out = render({1.0f, 0.0f, 0.0f, 8.0f});
        REQUIRE(measureFrequency(out, sampleRate) == Approx(220.0).epsilon(0.005));
    }

    SECTION("BLEND choisit le chemin, ORDER les met en série")
    {
        params.cutoff = 200.0f;
        params.vcfEg1Amt = 0.0f;
        params.fold = 0.8f;
        const dedale::VoiceCv open{0.0f, 0.0f, 0.0f, 8.0f};
        auto brightness = [&] { return distortion(render(open), 110.0); };

        params.blend = 0.0f;
        const auto folderOnly = brightness();
        params.blend = 1.0f;
        const auto filterOnly = brightness();
        REQUIRE(folderOnly > 4.0 * filterOnly);

        // VCW>VCF : le filtre reçoit le signal plié, donc la sortie filtre contient les plis filtrés.
        params.order = dedale::Order::vcwToVcf;
        const auto serial = brightness();
        REQUIRE(serial > filterOnly);
        REQUIRE(serial < folderOnly);
    }

    SECTION("aucune valeur non finie avec tout à fond")
    {
        params = {};
        params.fmAmt = params.mixVco = params.mixRing = params.mixMvco = params.mixNoise = 1.0f;
        params.fold = params.resonance = params.volume = 1.0f;
        params.bias = -1.0f;
        params.vcoFreq = 5000.0f;
        params.mvcoFreq = 1300.0f;
        for (const auto v : render({5.0f, -5.0f, 8.0f, 8.0f}))
        {
            REQUIRE(std::isfinite(v));
            REQUIRE(std::abs(v) <= 1.0f);
        }
    }
}
