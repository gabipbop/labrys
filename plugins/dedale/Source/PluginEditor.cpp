#include "PluginEditor.h"

#include "PluginProcessor.h"

DedaleEditor::DedaleEditor(DedaleProcessor& processor) : AudioProcessorEditor(processor)
{
    setSize(920, 400); // format paysage ~2,3:1 (§11.1)
}

void DedaleEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff26282b));

    auto area = getLocalBounds().reduced(24);
    g.setColour(juce::Colour(0xffd9772b));
    g.setFont(juce::FontOptions(48.0f, juce::Font::bold));
    g.drawText(juce::String::fromUTF8(LABRYS_DISPLAY_NAME), area.removeFromTop(64),
               juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff9aa0a6));
    g.setFont(juce::FontOptions(16.0f));
    g.drawText(juce::String::fromUTF8(
                   "Phase 2 : séquenceur + bip de debug. Réglages : paramètres du plugin dans la DAW."),
               area.removeFromTop(24), juce::Justification::centredLeft);
}
