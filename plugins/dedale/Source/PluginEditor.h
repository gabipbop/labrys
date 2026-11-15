#pragma once

#include "UI/Panel.h"

// Éditeur : met le panneau à l'échelle de la fenêtre (75 % à 200 %, proportions fixes).
class DedaleEditor final : public juce::AudioProcessorEditor
{
public:
    explicit DedaleEditor(DedaleProcessor& processor);
    ~DedaleEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshNow() { panel.refresh(); } // pour les captures hors temps réel

private:
    DedaleProcessor& dedale;
    dedale::ui::KnobLookAndFeel lookAndFeel;
    dedale::ui::Panel panel;
    juce::TooltipWindow tooltips{this, 700};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DedaleEditor)
};
