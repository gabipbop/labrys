#pragma once

#include <cmath>
#include <numbers>

namespace dedale
{
inline constexpr float kPi = std::numbers::pi_v<float>;

// Saturation douce bornée à ±ceiling, linéaire (gain 1) autour de zéro.
inline float softLimit(float x, float ceiling) noexcept
{
    return ceiling * std::tanh(x / ceiling);
}

// Passe-haut du premier ordre qui retire la composante continue.
class DcBlocker
{
public:
    void prepare(double sampleRate, float cutoffHz = 10.0f) noexcept
    {
        pole = 1.0f - 2.0f * kPi * cutoffHz / static_cast<float>(sampleRate);
        reset();
    }

    void reset() noexcept { previousInput = previousOutput = 0.0f; }

    float processSample(float x) noexcept
    {
        previousOutput = x - previousInput + pole * previousOutput;
        previousInput = x;
        return previousOutput;
    }

private:
    float pole = 0.999f;
    float previousInput = 0.0f;
    float previousOutput = 0.0f;
};
} // namespace dedale
