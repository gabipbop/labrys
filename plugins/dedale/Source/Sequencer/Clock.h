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

// Horloge MIDI (24 impulsions par noire). Un pas dure « division » doubles croches, soit 6 × division
// impulsions. Elle prend la main dès qu'une impulsion arrive, et la rend 5 s après la dernière (§8.6).
class MidiClock
{
public:
    void prepare(double newSampleRate) noexcept;
    // Start ou Song Position 0 : la prochaine impulsion donne un tick.
    void restart() noexcept
    {
        pulses = 0;
        arm();
    }
    // Start ou Continue : l'horloge MIDI prend la main tout de suite, sans attendre la première impulsion
    // (sinon l'horloge interne jouerait un pas de trop entre-temps).
    void arm() noexcept { samplesSincePulse = 0; }
    bool onPulse(int division) noexcept; // renvoie vrai si cette impulsion est un tick
    void advance() noexcept              // un échantillon passe
    {
        if (samplesSincePulse < timeoutSamples)
            ++samplesSincePulse;
    }
    bool isActive() const noexcept { return samplesSincePulse < timeoutSamples; }

private:
    long timeoutSamples = 240000;
    long samplesSincePulse = 240000; // inactive tant qu'aucune impulsion n'est arrivée
    int pulses = 0;
};
} // namespace dedale
