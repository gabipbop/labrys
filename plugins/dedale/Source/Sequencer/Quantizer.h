#pragma once

#include <array>
#include <initializer_list>

namespace dedale
{
inline constexpr int kNumQuantizerModes = 16;

struct Scale
{
    std::array<int, 12> degrees{}; // demi-tons depuis la fondamentale, croissants
    int size = 0;                  // 0 = non quantifié
};

// Gamme d'un mode (index 0..15, borné).
const Scale& scaleForMode(int mode) noexcept;

// Quantifie une tension (1 V/octave, 0 V = fondamentale) au degré le plus proche de la gamme du mode.
// rootSemitones transpose la fondamentale. Mode 0 : la tension passe telle quelle.
float quantize(float volts, int mode, int rootSemitones = 0) noexcept;
} // namespace dedale
