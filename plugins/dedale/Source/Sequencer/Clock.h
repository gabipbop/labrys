#pragma once

namespace dedale
{
// Horloge interne : un tick par pas, au tempo donné en pas par minute. Le premier tick tombe tout de suite.
class InternalClock
{
public:
    void prepare(double newSampleRate) noexcept;
    void reset() noexcept { phase = 1.0; }
    bool processSample(double stepsPerMinute) noexcept;

private:
    double sampleRate = 48000.0;
    double phase = 1.0;
};

// Horloge hôte : ticks dérivés de la position PPQ du transport, à l'échantillon près.
// Un tick tombe à chaque multiple de la durée d'un pas (division × 1/16 de ronde).
class HostClock
{
public:
    // À appeler quand le transport ne tourne pas : le prochain bloc repart de sa propre position.
    void reset() noexcept { continuous = false; }
    // Position PPQ du premier échantillon du bloc et avance par échantillon.
    void beginBlock(double ppqAtBlockStart, double ppqPerSample) noexcept;
    bool processSample(int division) noexcept;

private:
    double ppq = 0.0;
    double previousPpq = 0.0;
    double increment = 0.0;
    bool continuous = false;
};
} // namespace dedale
