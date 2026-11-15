#include "Quantizer.h"

#include <algorithm>
#include <cmath>

namespace dedale
{
namespace
{
constexpr Scale makeScale(std::initializer_list<int> degrees)
{
    Scale scale;
    for (const auto degree : degrees)
        scale.degrees[static_cast<std::size_t>(scale.size++)] = degree;
    return scale;
}

// Gammes des 16 modes (§8.5), en demi-tons depuis la fondamentale. Table à éditer librement.
constexpr std::array<Scale, kNumQuantizerModes> kScales{
    makeScale({}),                                     //  1 non quantifié
    makeScale({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}), //  2 chromatique
    makeScale({0, 2, 4, 5, 7, 9, 11}),                 //  3 majeur
    makeScale({0, 2, 4, 7, 9}),                        //  4 pentatonique
    makeScale({0, 2, 3, 5, 7, 9, 11}),                 //  5 mineur mélodique
    makeScale({0, 2, 3, 5, 7, 8, 11}),                 //  6 mineur harmonique
    makeScale({0, 2, 4, 5, 7, 8, 9, 11}),              //  7 diminished 6th        TODO calibrate
    makeScale({0, 2, 4, 6, 8, 10}),                    //  8 par tons
    makeScale({0, 2, 3, 7, 8}),                        //  9 hirajoshi             TODO calibrate
    makeScale({0, 5, 7, 10}),                          // 10 7sus4 (1 4 5 b7)
    makeScale({0, 4, 7, 11}),                          // 11 maj7 (1 3 5 7)
    makeScale({0, 2, 4, 7, 9, 11}),                    // 12 maj13 (1 3 5 6 7 9)
    makeScale({0, 3, 7, 10}),                          // 13 min7 (1 b3 5 b7)
    makeScale({0, 2, 3, 5, 7, 10}),                    // 14 min11 (1 b3 4 5 b7 9)
    makeScale({0, 2, 3, 5, 7, 8, 10}), // 15 accordage handpan     TODO calibrate (hypothèse : type « Kurd »)
    makeScale({0, 3, 6, 9}),           // 16 quads (tierces mineures)
};
} // namespace

const Scale& scaleForMode(int mode) noexcept
{
    return kScales[static_cast<std::size_t>(std::clamp(mode, 0, kNumQuantizerModes - 1))];
}

float quantize(float volts, int mode, int rootSemitones) noexcept
{
    const auto& scale = scaleForMode(mode);
    if (scale.size == 0)
        return volts;

    const auto semitones = static_cast<double>(volts) * 12.0 - rootSemitones;
    const auto octave = std::floor(semitones / 12.0);
    const auto remainder = semitones - octave * 12.0; // [0, 12)

    // Candidats : les degrés de l'octave, puis la fondamentale de l'octave suivante. À égalité, le plus
    // grave.
    auto best = 12.0;
    auto bestDistance = 12.0 - remainder;
    for (int i = scale.size - 1; i >= 0; --i)
    {
        const auto degree = static_cast<double>(scale.degrees[static_cast<std::size_t>(i)]);
        const auto distance = std::abs(remainder - degree);
        if (distance <= bestDistance)
        {
            best = degree;
            bestDistance = distance;
        }
    }
    return static_cast<float>((octave * 12.0 + best + rootSemitones) / 12.0);
}
} // namespace dedale
