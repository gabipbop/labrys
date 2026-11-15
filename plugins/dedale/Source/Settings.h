#pragma once

#include <algorithm>
#include <cstdint>

namespace dedale
{
// Réglages globaux (§8.8), hors paramètres automatisables : ils sont sauvegardés avec le projet.
struct GlobalSettings
{
    bool resetRecallsBuffer = false; // jack RESET : retour au pas 1, et rappel du buffer
    bool unipolarCv = false;         // CV des séquenceurs en 0 → +5 V au lieu de ±5 V
    bool analogDrift = true;         // très légère dérive lente de la hauteur des oscillateurs
    bool vcwZeroColour = true;       // coloration résiduelle du wavefolder à FOLD = 0
    bool midiOut = false;            // émet les triggers des séquenceurs en notes MIDI
    int oversampling = 4;            // 2 (éco), 4 (normal) ou 8 (haute qualité)
    int midiChannel = 0;             // 0 = omni, sinon 1 à 16
    bool operator==(const GlobalSettings&) const = default;
};

// Rangement dans un entier, pour un échange atomique entre l'interface et l'audio.
constexpr std::uint32_t pack(const GlobalSettings& s) noexcept
{
    const auto stages = s.oversampling >= 8 ? 3u : s.oversampling >= 4 ? 2u : 1u;
    return (s.resetRecallsBuffer ? 1u : 0u) | (s.unipolarCv ? 2u : 0u) | (s.analogDrift ? 4u : 0u) |
           (s.vcwZeroColour ? 8u : 0u) | (s.midiOut ? 16u : 0u) | (stages << 8) |
           (static_cast<std::uint32_t>(std::clamp(s.midiChannel, 0, 16)) << 16);
}

// Lecture tolérante : une valeur hors bornes est ramenée dans les bornes.
constexpr GlobalSettings unpack(std::uint32_t bits) noexcept
{
    GlobalSettings s;
    s.resetRecallsBuffer = (bits & 1u) != 0;
    s.unipolarCv = (bits & 2u) != 0;
    s.analogDrift = (bits & 4u) != 0;
    s.vcwZeroColour = (bits & 8u) != 0;
    s.midiOut = (bits & 16u) != 0;
    s.oversampling = 1 << std::clamp(static_cast<int>((bits >> 8) & 15u), 1, 3);
    s.midiChannel = std::clamp(static_cast<int>((bits >> 16) & 31u), 0, 16);
    return s;
}
} // namespace dedale
