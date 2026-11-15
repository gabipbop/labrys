#include "ButtonComboHandler.h"

namespace dedale
{
ActionList ButtonComboHandler::press(PanelButton button) noexcept
{
    ActionList actions;
    held[index(button)] = true;
    consumed[index(button)] = false;

    const auto shiftHeld = [this](int lane) { return isHeld(forLane(PanelButton::bitShift1, lane)); };
    const auto laneOf = [](PanelButton b, PanelButton first)
    { return static_cast<int>(b) - static_cast<int>(first); };

    switch (button)
    {
    case PanelButton::reset:
    {
        for (int lane = 0; lane < 2; ++lane)
        {
            if (shiftHeld(lane))
            {
                actions.add(
                    {isHeld(PanelButton::advance) ? ActionType::writeHeadReset : ActionType::bitShiftReset,
                     lane});
                consume(forLane(PanelButton::bitShift1, lane));
            }
            else if (isHeld(forLane(PanelButton::length1, lane)))
            {
                actions.add({ActionType::lengthReset, lane});
                consume(forLane(PanelButton::length1, lane));
            }
        }
        if (isHeld(PanelButton::buffer) && actions.count == 0)
        {
            bufferHeldSeconds = 0.0; // BUFFER + RESET : l'effacement part après 1 s (advanceTime)
            clearArmed = true;
        }
        else if (actions.count == 0)
            actions.add({ActionType::reset});
        break;
    }
    case PanelButton::advance:
    {
        for (int lane = 0; lane < 2; ++lane)
        {
            if (shiftHeld(lane))
            {
                actions.add({ActionType::bitShiftAdvance, lane});
                consume(forLane(PanelButton::bitShift1, lane));
            }
        }
        if (actions.count == 0)
            actions.add({ActionType::advance});
        break;
    }
    case PanelButton::bitFlip1:
    case PanelButton::bitFlip2:
    {
        const auto lane = laneOf(button, PanelButton::bitFlip1);
        if (shiftHeld(lane))
        {
            actions.add({lane == 0 ? ActionType::quantizerNext : ActionType::clockDivisionUp});
            consume(forLane(PanelButton::bitShift1, lane));
            consume(button);
        }
        else
        {
            actions.add({ActionType::bitFlipGate, lane, true});
        }
        break;
    }
    case PanelButton::length1:
    case PanelButton::length2:
    {
        const auto lane = laneOf(button, PanelButton::length1);
        if (shiftHeld(lane))
        {
            actions.add({lane == 0 ? ActionType::quantizerPrevious : ActionType::clockDivisionDown});
            consume(forLane(PanelButton::bitShift1, lane));
            consume(button);
        }
        break;
    }
    case PanelButton::buffer:
        bufferHeldSeconds = 0.0;
        clearArmed = isHeld(PanelButton::reset);
        break;
    case PanelButton::trigger:
        actions.add({ActionType::trigger});
        break;
    case PanelButton::bitShift1:
    case PanelButton::bitShift2:
    case PanelButton::count:
        break;
    }
    return actions;
}

ActionList ButtonComboHandler::release(PanelButton button) noexcept
{
    ActionList actions;
    if (!isHeld(button))
        return actions;
    held[index(button)] = false;
    if (consumed[index(button)])
        return actions;

    switch (button)
    {
    case PanelButton::bitFlip1:
    case PanelButton::bitFlip2:
        actions.add({ActionType::bitFlipGate,
                     static_cast<int>(button) - static_cast<int>(PanelButton::bitFlip1), false});
        break;
    case PanelButton::bitShift1:
    case PanelButton::bitShift2:
        actions.add(
            {ActionType::bitShift, static_cast<int>(button) - static_cast<int>(PanelButton::bitShift1)});
        break;
    case PanelButton::length1:
    case PanelButton::length2:
        actions.add(
            {ActionType::lengthPress, static_cast<int>(button) - static_cast<int>(PanelButton::length1)});
        break;
    case PanelButton::buffer:
        // Relâché avant l'appui long. Après une tentative d'effacement (BUFFER + RESET), on ne rappelle rien.
        if (!clearArmed)
            actions.add({ActionType::bufferRecall});
        break;
    case PanelButton::reset:
    case PanelButton::advance:
    case PanelButton::trigger:
    case PanelButton::count:
        break;
    }
    return actions;
}

ActionList ButtonComboHandler::advanceTime(double seconds) noexcept
{
    ActionList actions;
    if (!isHeld(PanelButton::buffer) || consumed[index(PanelButton::buffer)])
        return actions;

    bufferHeldSeconds += seconds;
    if (bufferHeldSeconds >= kLongPressSeconds)
    {
        if (!clearArmed)
            actions.add({ActionType::bufferSave});
        else if (isHeld(PanelButton::reset))
            actions.add({ActionType::clear});
        consume(PanelButton::buffer);
    }
    return actions;
}

ButtonComboHandler::DisplayMode ButtonComboHandler::displayMode() const noexcept
{
    if (isHeld(PanelButton::bitShift1))
        return DisplayMode::quantizer;
    if (isHeld(PanelButton::bitShift2))
        return DisplayMode::clockDivision;
    return DisplayMode::sequence;
}

// Abandonne tous les appuis en cours sans déclencher leurs actions de relâchement. L'appelant referme
// lui-même les portes BIT FLIP encore ouvertes (il sait lesquelles étaient tenues).
void ButtonComboHandler::releaseAll() noexcept
{
    held = {};
    consumed = {};
    bufferHeldSeconds = 0.0;
    clearArmed = false;
}
} // namespace dedale
