#pragma once

#include "../PluginProcessor.h"
#include "Style.h"

namespace dedale::ui
{
// Extrémité d'un câble : une prise d'entrée ou de sortie.
struct Jack
{
    bool isOutput = false;
    int index = -1;
    bool isValid() const noexcept { return index >= 0; }
    bool operator==(const Jack&) const = default;
};

juce::Point<float> jackCentre(Jack jack) noexcept; // dans le repère du panneau
juce::Colour cableColour(int output) noexcept;     // couleur par type de signal : audio, CV, trigger

class PatchBayView;

// Calque des câbles, au-dessus de tout le panneau : il ne reçoit aucun clic.
class CableOverlay final : public juce::Component
{
public:
    CableOverlay(DedaleProcessor& processorToShow, PatchBayView& patchBay);
    void refresh(); // redessine si un câble, le survol ou le glisser a changé
    void paint(juce::Graphics&) override;

private:
    DedaleProcessor& processor;
    PatchBayView& view;
    juce::String shown;
};

// Les 32 prises. Glisser d'une prise à une autre pose un câble ; glisser depuis une entrée déjà câblée le
// déplace ; clic droit ou double-clic sur une prise retire ses câbles.
class PatchBayView final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit PatchBayView(DedaleProcessor& processorToControl);
    void paint(juce::Graphics&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    Jack hovered;                    // prise survolée
    Jack dragFrom;                   // prise d'où part le câble en cours de pose
    juce::Point<float> dragPosition; // extrémité libre, dans le repère du panneau

private:
    Jack jackAt(juce::Point<float> panelPosition) const noexcept;
    juce::Point<float> toPanel(const juce::MouseEvent& event) const noexcept;
    void removeCablesAt(Jack jack);

    DedaleProcessor& processor;
};
} // namespace dedale::ui
