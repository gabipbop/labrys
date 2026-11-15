#pragma once

#include "PatchBayView.h"
#include "Widgets.h"

namespace dedale::ui
{
// Le panneau complet, dessiné dans le repère fixe 960 × 444 (l'éditeur le met à l'échelle).
class Panel final : public juce::Component, private juce::Timer
{
public:
    Panel(DedaleProcessor& processorToControl, juce::Component& popupParent);
    ~Panel() override;

    // Met à jour LEDs, afficheur, oscilloscope et câbles, et fait avancer les appuis longs. Appelé 30 fois
    // par seconde ; public pour les captures hors temps réel.
    void refresh();

private:
    // Façade statique (sérigraphie, cadres, flancs), mise en cache dans une image.
    struct Background final : public juce::Component
    {
        void paint(juce::Graphics&) override;
    };

    void timerCallback() override { refresh(); }
    void perform(const ActionList& actions);
    void comboPressed(PanelButton button);
    void comboReleased(PanelButton button);
    SquareButton& addButton(std::unique_ptr<SquareButton>& slot, const char* tooltip, float x, float y,
                            float width);

    DedaleProcessor& processor;
    ButtonComboHandler handler;
    int advanceGrace =
        0; // en mode SHIFT, ADVANCE reste tenu quelques instants pour la combinaison à trois boutons

    Background background;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::array<std::unique_ptr<SquareButton>, static_cast<std::size_t>(PanelButton::count)> comboButtons;
    std::array<bool, static_cast<std::size_t>(PanelButton::count)> comboDown{};
    std::unique_ptr<SquareButton> runButton, chainButton, syncButton, shiftButton;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> attachments;
    SequencerLeds leds;
    ModeDisplay display;
    OrderSwitch order;
    ScopeView scope;
    PatchBayView patchBay;
    CableOverlay cables;
};
} // namespace dedale::ui
