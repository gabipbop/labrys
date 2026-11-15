#pragma once

#include "../PluginProcessor.h"
#include "Style.h"

namespace dedale::ui
{
// Knob classique : graduations, jupe noire cannelée, capuchon aluminium, trait blanc.
// Les petits rayons (< 9) sont dessinés en trimmers, sans capuchon ni graduations.
class KnobLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float position,
                          float startAngle, float endAngle, juce::Slider&) override;
    juce::Font getPopupMenuFont() override { return font(15.0f); }
    juce::Font getSliderPopupFont(juce::Slider&) override { return font(15.0f, true); }
};

class Knob final : public juce::Slider
{
public:
    Knob(juce::AudioProcessorValueTreeState& state, const char* parameterId, float radius,
         const char* tooltip, juce::Component& popupParent);
    float radius;

private:
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// Bouton carré lumineux. Momentané par défaut ; setClickingTogglesState(true) en fait un interrupteur.
class SquareButton final : public juce::Button
{
public:
    explicit SquareButton(const juce::String& tooltip);
    void setLit(bool shouldBeLit);

private:
    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
    bool lit = false;
};

// Les deux rangées de 8 LEDs : vert = play head, rouge = pas actif, anneau cyan = write head.
// Pendant un réglage (BIT SHIFT tenu), les 16 LEDs montrent le mode du quantizer ou la division d'horloge.
class SequencerLeds final : public juce::Component
{
public:
    explicit SequencerLeds(DedaleProcessor& processorToShow) : processor(processorToShow) {}
    void refresh(ButtonComboHandler::DisplayMode mode, int clockDivision);
    void paint(juce::Graphics&) override;

private:
    DedaleProcessor& processor;
    juce::uint64 shown = ~juce::uint64{0};
    ButtonComboHandler::DisplayMode displayMode = ButtonComboHandler::DisplayMode::sequence;
    int division = 1;
};

// Afficheur : mode du quantizer, ou division d'horloge pendant son réglage.
class ModeDisplay final : public juce::Component
{
public:
    void show(const juce::String& newText);
    void paint(juce::Graphics&) override;

private:
    juce::String text;
};

// Sélecteur ORDER à trois positions.
class OrderSwitch final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit OrderSwitch(juce::RangedAudioParameter& parameter);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::ParameterAttachment attachment;
    int position = 0;
};

// Petit oscilloscope de la sortie. Un clic l'éteint ou le rallume.
class ScopeView final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ScopeView(DedaleProcessor& processorToShow);
    void refresh();
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    DedaleProcessor& processor;
    std::array<float, 2048> samples{};
    bool enabled = true;
};

const char* quantizerName(int mode) noexcept;
} // namespace dedale::ui
