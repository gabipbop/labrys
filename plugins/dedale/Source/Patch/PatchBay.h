#pragma once

#include <array>
#include <cstdint>

namespace dedale
{
// Les 20 entrées de la patch bay (§9), dans l'ordre du panneau.
enum class In : std::uint8_t
{
    vcoPitch,  // VCO 1V/OCT, sommé au knob
    mvcoSync,  // M VCO SYNC, gate
    mvcoPitch, // M VCO 1V/OCT
    blend,     // sommé au knob
    vcwIn,     // normalisé sur le mixeur (ou le VCF selon ORDER)
    vcwFold,   // normalisé sur EG1
    vcwVcaCv,  // normalisé sur EG2
    vcfIn,     // normalisé sur le mixeur (ou le VCW selon ORDER)
    cutoff,    // normalisé sur EG1
    vcfVcaCv,  // normalisé sur EG2
    umix1,     // normalisé sur le ring mod
    umix2,
    eg2Trig,
    clock1, // normalisé sur l'horloge interne/hôte
    bitFlip1,
    clock2, // normalisé sur CLOCK 1
    bitFlip2,
    midi, // représenté sur le panneau, pas câblable
    trigger,
    reset
};
inline constexpr int kNumInputs = 20;

// Les 12 sorties.
enum class Out : std::uint8_t
{
    vca, // sortie principale
    mvco,
    noise,
    mixer,
    eg1,
    eg2,
    umix, // U MIX 1+2
    seq1Cv,
    seq1Trig,
    seq2Cv,
    seq2Trig,
    clock
};
inline constexpr int kNumOutputs = 12;

// Routeur de la patch bay : chaque jack est une tension (en volts) par échantillon.
// Une sortie peut alimenter plusieurs entrées, une entrée n'accepte qu'un câble, et la patcher casse sa
// normalisation. Une entrée lit la dernière valeur écrite sur la sortie reliée : si cette sortie n'a pas
// encore été recalculée dans l'échantillon courant, c'est celle de l'échantillon précédent (feedback à un
// échantillon).
class PatchBay
{
public:
    PatchBay() noexcept { clear(); }

    // Relie une sortie à une entrée (remplace le câble déjà présent). Faux si l'entrée n'est pas câblable.
    bool connect(Out output, In input) noexcept
    {
        if (input == In::midi)
            return false;
        sources[index(input)] = static_cast<std::int8_t>(output);
        return true;
    }

    void disconnect(In input) noexcept { sources[index(input)] = -1; }
    void clear() noexcept { sources.fill(-1); }
    void resetSignals() noexcept { voltages.fill(0.0f); }

    bool isPatched(In input) const noexcept { return sources[index(input)] >= 0; }
    // Sortie reliée à l'entrée, ou −1.
    int source(In input) const noexcept { return sources[index(input)]; }

    // Tension vue par une entrée : celle du câble, ou la valeur de normalisation si rien n'est patché.
    float read(In input, float normalled) const noexcept
    {
        const auto from = sources[index(input)];
        return from < 0 ? normalled : voltages[static_cast<std::size_t>(from)];
    }

    void write(Out output, float volts) noexcept { voltages[static_cast<std::size_t>(output)] = volts; }
    float output(Out output) const noexcept { return voltages[static_cast<std::size_t>(output)]; }

private:
    static std::size_t index(In input) noexcept { return static_cast<std::size_t>(input); }

    std::array<std::int8_t, kNumInputs> sources{};
    std::array<float, kNumOutputs> voltages{};
};

// Entrée trigger/gate (§9) : passe à l'état haut au-dessus de +1 V, redescend sous +0,5 V.
class GateDetector
{
public:
    // Renvoie vrai sur le front montant.
    bool process(float volts) noexcept
    {
        const auto wasHigh = high;
        if (volts > 1.0f)
            high = true;
        else if (volts < 0.5f)
            high = false;
        return high && !wasHigh;
    }

    bool isHigh() const noexcept { return high; }
    void reset() noexcept { high = false; }

private:
    bool high = false;
};
} // namespace dedale
