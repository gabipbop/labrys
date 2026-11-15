#pragma once

#include "Rng.h"

#include <array>
#include <cstdint>

namespace dedale
{
inline constexpr int kNumLanes = 2;
inline constexpr int kMaxSteps = 8;

struct SeqLane
{
    std::array<bool, kMaxSteps> bit{};      // pas actif
    std::array<float, kMaxSteps> voltage{}; // tension stockée, -5..+5 V
    int length = kMaxSteps;                 // 1..8
    int playHead = 0;                       // position de lecture (0..15 en mode chain)
    int writeOffset = 0;                    // décalage du write head par rapport au play head
    float corrupt = 0.0f;                   // 0..1
    float cvRange = 1.0f;                   // 0..1
};

// Séquenceur génératif double (§8) : deux lanes de 8 pas à registre à décalage, chaînables en 16 pas.
// Classe pure : aucun JUCE, aucune allocation, RNG seedable. Chaque lane a sa propre horloge (tick).
// Les combinaisons de boutons (§8.4) sont traduites en appels de ces méthodes par l'UI (phase 5).
class GenerativeSequencer
{
public:
    explicit GenerativeSequencer(std::uint32_t seedValue = 1) noexcept;

    void seed(std::uint32_t seedValue) noexcept { rng.seed(seedValue); }

    // --- Horloge et transport ---
    // Front d'horloge d'une lane. Renvoie true si le pas lu déclenche un trigger. Sans effet à l'arrêt.
    bool tick(int lane) noexcept;
    void setRunning(bool shouldRun) noexcept { running = shouldRun; }
    bool isRunning() const noexcept { return running; }
    // RESET : play heads au pas 1 (joué au prochain tick), offsets du write head conservés.
    void reset() noexcept;
    // ADVANCE : avance les deux lanes d'un pas, seulement à l'arrêt. Renvoie les triggers des pas atteints.
    std::array<bool, kNumLanes> advance() noexcept;

    // --- Actions ---
    // BIT FLIP (bouton ou jack) : le front montant inverse le bit au write head ; tant que la porte reste
    // haute, chaque bit est inversé au passage du write head.
    void setBitFlipGate(int lane, bool high) noexcept;
    void pressLength(int lane) noexcept; // 8 → 7 → … → 1 → 8
    void resetLength(int lane) noexcept; // LENGTH + RESET
    // BIT SHIFT : rotation d'un pas vers la droite dans la longueur courante. En mode chain, la lane 0
    // tourne les 16 pas et la lane 1 avance seulement son play head.
    void bitShift(int lane) noexcept;
    void undoBitShift(int lane) noexcept;   // BIT SHIFT + RESET
    void shiftWriteHead(int lane) noexcept; // BIT SHIFT + ADVANCE
    void resetWriteHead(int lane) noexcept; // BIT SHIFT + ADVANCE + RESET
    void syncChainPlayHeads() noexcept;     // BIT SHIFT 2 + RESET en mode chain
    void setChain(bool shouldChain) noexcept;
    bool isChained() const noexcept { return chained; }

    void setQuantizerMode(int mode) noexcept;
    int quantizerMode() const noexcept { return quantMode; }
    void nextQuantizerMode() noexcept { setQuantizerMode((quantMode + 1) % 16); }
    void previousQuantizerMode() noexcept { setQuantizerMode((quantMode + 15) % 16); }

    // BUFFER : un seul slot (bits, tensions, quantizer, offsets, longueurs, chain).
    void saveBuffer() noexcept;
    void recallBuffer() noexcept; // sans effet si rien n'a été sauvegardé
    bool hasBuffer() const noexcept { return buffer.valid; }
    void clear() noexcept; // BUFFER + RESET 1 s : bits, quantizer, offsets

    // --- Réglages ---
    void setCorrupt(int lane, float amount) noexcept;
    void setCvRange(int lane, float amount) noexcept;
    void setUnipolar(bool shouldBeUnipolar) noexcept { unipolar = shouldBeUnipolar; }
    void setRootSemitones(int semitones) noexcept { rootSemitones = semitones; }
    void setStep(int lane, int index, bool on, float volts) noexcept;

    // --- Lecture ---
    // CV de sortie : tension tenue × CV RANGE, polarité, puis quantizer.
    float cv(int lane) const noexcept;
    const SeqLane& lane(int index) const noexcept { return lanes[laneIndex(index)]; }
    int span(int lane) const noexcept; // nombre de pas parcourus : longueur, ou somme des deux en chain
    int writeHead(int lane) const noexcept;

private:
    struct StepRef
    {
        int lane;
        int index;
    };

    struct Buffer
    {
        std::array<std::array<bool, kMaxSteps>, kNumLanes> bit{};
        std::array<std::array<float, kMaxSteps>, kNumLanes> voltage{};
        std::array<int, kNumLanes> length{};
        std::array<int, kNumLanes> writeOffset{};
        int quantMode = 0;
        bool chained = false;
        bool valid = false;
    };

    static std::size_t laneIndex(int lane) noexcept { return lane == 0 ? 0u : 1u; }
    StepRef resolve(int lane, int position) const noexcept;
    bool step(int lane, bool applyCorrupt) noexcept;
    void flipAt(int lane, int position) noexcept;
    void corruptWriteHead(int lane) noexcept;
    bool readPlayHead(int lane) noexcept;
    void rotate(int lane, bool right) noexcept;
    void wrapHeads() noexcept;
    void forgetRotations() noexcept { rotations = {}; }

    std::array<SeqLane, kNumLanes> lanes{};
    std::array<float, kNumLanes> heldVolts{}; // sample & hold de la dernière tension lue
    std::array<bool, kNumLanes> flipGate{};
    std::array<bool, kNumLanes> playCurrentStepNext{true,
                                                    true}; // après RESET : le prochain tick joue le pas 1
    std::array<int, kNumLanes> rotations{};                // rotations à annuler par undoBitShift
    Buffer buffer{};
    Rng rng;
    int quantMode;
    int rootSemitones = 0;
    bool running = false;
    bool chained = false;
    bool unipolar = false;
};
} // namespace dedale
