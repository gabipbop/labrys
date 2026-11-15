#pragma once

// Outils de mesure partagés par les tests DSP et les tests de la patch bay.

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace test
{
inline constexpr double sampleRate = 192000.0; // la voix tourne suréchantillonnée ×4
inline constexpr float pi = std::numbers::pi_v<float>;

// Fréquence mesurée par les passages à zéro montants, avec interpolation linéaire.
inline double measureFrequency(const std::vector<float>& x, double rate)
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

inline double rms(const std::vector<float>& x, std::size_t from = 0)
{
    double sum = 0.0;
    for (std::size_t i = from; i < x.size(); ++i)
        sum += static_cast<double>(x[i]) * x[i];
    return std::sqrt(sum / static_cast<double>(x.size() - from));
}

inline float peak(const std::vector<float>& x)
{
    float p = 0.0f;
    for (const auto v : x)
        p = std::max(p, std::abs(v));
    return p;
}

// Amplitude de la composante à la fréquence donnée (corrélation avec sinus et cosinus).
inline double componentAmplitude(const std::vector<float>& x, double frequency, double rate)
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

inline std::vector<float> sine(double frequency, float amplitude, int samples)
{
    std::vector<float> x(static_cast<std::size_t>(samples));
    for (std::size_t i = 0; i < x.size(); ++i)
        x[i] = amplitude * static_cast<float>(std::sin(2.0 * std::numbers::pi * frequency *
                                                       static_cast<double>(i) / sampleRate));
    return x;
}

// Taux de distorsion : tout ce qui n'est pas la fondamentale, rapporté à la fondamentale.
inline double distortion(const std::vector<float>& x, double frequency)
{
    const auto fundamental = componentAmplitude(x, frequency, sampleRate) / std::sqrt(2.0);
    const auto total = rms(x);
    return std::sqrt(std::max(0.0, total * total - fundamental * fundamental)) / fundamental;
}
} // namespace test
