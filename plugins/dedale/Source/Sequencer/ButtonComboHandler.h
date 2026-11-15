#pragma once

#include <array>
#include <cstdint>

namespace dedale
{
// Boutons du panneau qui participent aux combinaisons du §8.4.
enum class PanelButton : std::uint8_t
{
    bitFlip1,
    bitFlip2,
    bitShift1,
    bitShift2,
    length1,
    length2,
    reset,
    advance,
    buffer,
    trigger,
    count
};

enum class ActionType : std::uint8_t
{
    bitFlipGate,     // lane, on
    bitShift,        // lane
    bitShiftReset,   // lane : annule les rotations (ou resynchronise SEQ2 en mode chain)
    bitShiftAdvance, // lane : décale le write head (ou avance le play head de SEQ2 en mode chain)
    writeHeadReset,  // lane
    lengthPress,     // lane
    lengthReset,     // lane
    reset,
    advance,
    bufferSave,
    bufferRecall,
    clear,
    trigger,
    quantizerNext,
    quantizerPrevious,
    clockDivisionUp,
    clockDivisionDown
};

struct Action
{
    ActionType type;
    int lane = 0;
    bool on = false;
    bool operator==(const Action&) const = default;
};

// Actions produites par un événement (au plus deux : une par lane).
struct ActionList
{
    std::array<Action, 2> items{};
    int count = 0;

    void add(Action action) noexcept
    {
        if (count < static_cast<int>(items.size()))
            items[static_cast<std::size_t>(count++)] = action;
    }
    const Action* begin() const noexcept { return items.data(); }
    const Action* end() const noexcept { return items.data() + count; }
};

// Traduit les appuis sur les boutons du panneau en actions, combinaisons comprises (§8.4).
// Modèle : celui de boutons physiques. Une combinaison part à l'appui du dernier bouton, les autres étant
// tenus. Un bouton qui sert aussi de touche de combinaison (BIT SHIFT, LENGTH, BUFFER) n'agit qu'au
// relâchement, et seulement s'il n'a pas servi à une combinaison. Classe pure, pilotée depuis le thread de
// l'interface.
class ButtonComboHandler
{
public:
    enum class DisplayMode
    {
        sequence,     // les LEDs montrent la séquence
        quantizer,    // BIT SHIFT 1 tenu : mode du quantizer (1 à 16)
        clockDivision // BIT SHIFT 2 tenu : division d'horloge (1 à 16)
    };

    ActionList press(PanelButton button) noexcept;
    ActionList release(PanelButton button) noexcept;
    // À appeler régulièrement : gère l'appui long de BUFFER (1 s : sauvegarde ; avec RESET : effacement).
    ActionList advanceTime(double seconds) noexcept;

    bool isHeld(PanelButton button) const noexcept { return held[index(button)]; }
    DisplayMode displayMode() const noexcept;
    void releaseAll() noexcept; // abandonne les appuis en cours sans produire d'action

    static constexpr double kLongPressSeconds = 1.0;

private:
    static std::size_t index(PanelButton button) noexcept { return static_cast<std::size_t>(button); }
    static PanelButton forLane(PanelButton first, int lane) noexcept
    {
        return static_cast<PanelButton>(static_cast<int>(first) + lane);
    }
    void consume(PanelButton button) noexcept { consumed[index(button)] = true; }

    std::array<bool, static_cast<std::size_t>(PanelButton::count)> held{};
    std::array<bool, static_cast<std::size_t>(PanelButton::count)> consumed{};
    double bufferHeldSeconds = 0.0;
    bool clearArmed = false; // RESET a rejoint BUFFER : l'appui long efface au lieu de sauvegarder
};
} // namespace dedale
