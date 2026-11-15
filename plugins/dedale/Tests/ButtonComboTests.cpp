#include "Sequencer/ButtonComboHandler.h"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using dedale::Action;
using dedale::ActionType;
using dedale::ButtonComboHandler;
using dedale::PanelButton;

namespace
{
std::vector<Action> list(const dedale::ActionList& actions)
{
    return {actions.begin(), actions.end()};
}

// Appui bref : appui puis relâchement.
std::vector<Action> tap(ButtonComboHandler& handler, PanelButton button)
{
    auto actions = list(handler.press(button));
    for (const auto& action : handler.release(button))
        actions.push_back(action);
    return actions;
}

using Actions = std::vector<Action>;
} // namespace

TEST_CASE("Boutons : appuis simples", "[combo]")
{
    ButtonComboHandler handler;

    SECTION("BIT FLIP ouvre la porte à l'appui et la ferme au relâchement")
    {
        REQUIRE(list(handler.press(PanelButton::bitFlip2)) == Actions{{ActionType::bitFlipGate, 1, true}});
        REQUIRE(handler.isHeld(PanelButton::bitFlip2));
        REQUIRE(list(handler.release(PanelButton::bitFlip2)) == Actions{{ActionType::bitFlipGate, 1, false}});
    }

    SECTION("BIT SHIFT et LENGTH agissent au relâchement")
    {
        REQUIRE(list(handler.press(PanelButton::bitShift1)).empty());
        REQUIRE(list(handler.release(PanelButton::bitShift1)) == Actions{{ActionType::bitShift, 0}});
        REQUIRE(tap(handler, PanelButton::length2) == Actions{{ActionType::lengthPress, 1}});
    }

    SECTION("RESET, ADVANCE et TRIGGER agissent à l'appui")
    {
        REQUIRE(tap(handler, PanelButton::reset) == Actions{{ActionType::reset}});
        REQUIRE(tap(handler, PanelButton::advance) == Actions{{ActionType::advance}});
        REQUIRE(tap(handler, PanelButton::trigger) == Actions{{ActionType::trigger}});
    }

    SECTION("relâcher un bouton qui n'est pas tenu ne fait rien")
    {
        REQUIRE(list(handler.release(PanelButton::bitShift1)).empty());
    }
}

TEST_CASE("Boutons : BUFFER, appui court et appui long", "[combo]")
{
    ButtonComboHandler handler;

    SECTION("appui court : rappel au relâchement")
    {
        REQUIRE(list(handler.press(PanelButton::buffer)).empty());
        REQUIRE(list(handler.advanceTime(0.4)).empty());
        REQUIRE(list(handler.release(PanelButton::buffer)) == Actions{{ActionType::bufferRecall}});
    }

    SECTION("tenu 1 s : sauvegarde, une seule fois, et pas de rappel au relâchement")
    {
        handler.press(PanelButton::buffer);
        REQUIRE(list(handler.advanceTime(0.6)).empty());
        REQUIRE(list(handler.advanceTime(0.5)) == Actions{{ActionType::bufferSave}});
        REQUIRE(list(handler.advanceTime(2.0)).empty());
        REQUIRE(list(handler.release(PanelButton::buffer)).empty());
    }

    SECTION("BUFFER + RESET tenus 1 s : effacement, sans reset ni sauvegarde")
    {
        handler.press(PanelButton::buffer);
        handler.advanceTime(0.8);
        REQUIRE(list(handler.press(PanelButton::reset)).empty());
        REQUIRE(list(handler.advanceTime(0.5)).empty()); // le délai repart de l'appui sur RESET
        REQUIRE(list(handler.advanceTime(0.6)) == Actions{{ActionType::clear}});
        REQUIRE(list(handler.release(PanelButton::reset)).empty());
        REQUIRE(list(handler.release(PanelButton::buffer)).empty());
    }

    SECTION("BUFFER + RESET relâchés trop tôt : rien")
    {
        handler.press(PanelButton::buffer);
        handler.press(PanelButton::reset);
        handler.advanceTime(0.5);
        REQUIRE(list(handler.release(PanelButton::reset)).empty());
        REQUIRE(list(handler.release(PanelButton::buffer)).empty());
    }
}

TEST_CASE("Boutons : combinaisons du §8.4", "[combo]")
{
    ButtonComboHandler handler;

    SECTION("LENGTH + RESET : longueur remise à 8, sans reset ni raccourcissement")
    {
        handler.press(PanelButton::length1);
        REQUIRE(list(handler.press(PanelButton::reset)) == Actions{{ActionType::lengthReset, 0}});
        REQUIRE(list(handler.release(PanelButton::reset)).empty());
        REQUIRE(list(handler.release(PanelButton::length1)).empty());
    }

    SECTION("BIT SHIFT + RESET : annule les rotations, sans rotation au relâchement")
    {
        handler.press(PanelButton::bitShift2);
        REQUIRE(list(handler.press(PanelButton::reset)) == Actions{{ActionType::bitShiftReset, 1}});
        handler.release(PanelButton::reset);
        REQUIRE(list(handler.release(PanelButton::bitShift2)).empty());
    }

    SECTION("BIT SHIFT + ADVANCE : décale le write head, répétable sans relâcher BIT SHIFT")
    {
        handler.press(PanelButton::bitShift1);
        REQUIRE(tap(handler, PanelButton::advance) == Actions{{ActionType::bitShiftAdvance, 0}});
        REQUIRE(tap(handler, PanelButton::advance) == Actions{{ActionType::bitShiftAdvance, 0}});
        REQUIRE(list(handler.release(PanelButton::bitShift1)).empty());
    }

    SECTION("BIT SHIFT + ADVANCE + RESET : write head ramené sur le play head")
    {
        handler.press(PanelButton::bitShift1);
        handler.press(PanelButton::advance);
        REQUIRE(list(handler.press(PanelButton::reset)) == Actions{{ActionType::writeHeadReset, 0}});
    }

    SECTION("BIT SHIFT 1 + BIT FLIP 1 / LENGTH 1 : quantizer suivant / précédent")
    {
        handler.press(PanelButton::bitShift1);
        REQUIRE(handler.displayMode() == ButtonComboHandler::DisplayMode::quantizer);
        REQUIRE(tap(handler, PanelButton::bitFlip1) ==
                Actions{{ActionType::quantizerNext}}); // pas de porte BIT FLIP
        REQUIRE(tap(handler, PanelButton::bitFlip1) == Actions{{ActionType::quantizerNext}});
        REQUIRE(tap(handler, PanelButton::length1) == Actions{{ActionType::quantizerPrevious}});
        REQUIRE(list(handler.release(PanelButton::bitShift1)).empty());
        REQUIRE(handler.displayMode() == ButtonComboHandler::DisplayMode::sequence);
    }

    SECTION("BIT SHIFT 2 + BIT FLIP 2 / LENGTH 2 : division d'horloge + / −")
    {
        handler.press(PanelButton::bitShift2);
        REQUIRE(handler.displayMode() == ButtonComboHandler::DisplayMode::clockDivision);
        REQUIRE(tap(handler, PanelButton::bitFlip2) == Actions{{ActionType::clockDivisionUp}});
        REQUIRE(tap(handler, PanelButton::length2) == Actions{{ActionType::clockDivisionDown}});
    }

    SECTION("les combinaisons ne croisent pas les lanes")
    {
        handler.press(PanelButton::bitShift1);
        REQUIRE(list(handler.press(PanelButton::bitFlip2)) == Actions{{ActionType::bitFlipGate, 1, true}});
        REQUIRE(tap(handler, PanelButton::length2) == Actions{{ActionType::lengthPress, 1}});
    }

    SECTION("après une combinaison, les boutons retrouvent leur rôle normal")
    {
        handler.press(PanelButton::bitShift1);
        tap(handler, PanelButton::advance);
        handler.release(PanelButton::bitShift1);
        REQUIRE(tap(handler, PanelButton::bitShift1) == Actions{{ActionType::bitShift, 0}});
        REQUIRE(tap(handler, PanelButton::advance) == Actions{{ActionType::advance}});
    }
}

TEST_CASE("Boutons : tout relâcher d'un coup (bouton SHIFT éteint)", "[combo]")
{
    ButtonComboHandler handler;
    handler.press(PanelButton::bitShift1);
    handler.press(PanelButton::bitFlip2);
    handler.press(PanelButton::buffer);
    handler.releaseAll();
    for (int i = 0; i < static_cast<int>(PanelButton::count); ++i)
        REQUIRE_FALSE(handler.isHeld(static_cast<PanelButton>(i)));
    REQUIRE(list(handler.advanceTime(5.0)).empty()); // plus d'appui long en cours
    REQUIRE(tap(handler, PanelButton::bitShift1) == Actions{{ActionType::bitShift, 0}});
}
