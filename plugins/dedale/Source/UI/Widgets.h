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

// Petit bouton d'en-tête dessiné par un pictogramme (flèches des presets, engrenage des réglages).
class IconButton final : public juce::Button
{
public:
    enum class Icon
    {
        previous,
        next,
        gear
    };
    IconButton(Icon iconToDraw, const juce::String& tooltip);

private:
    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
    Icon icon;
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
    std::uint32_t lastTick = 0;
    int flashFrames = 0; // le repère du write head s'allume à chaque tick d'horloge, puis retombe
};

// Afficheur : mode du quantizer, ou division d'horloge pendant son réglage.
class ModeDisplay final : public juce::Component, public juce::SettableTooltipClient
{
public:
    void show(const juce::String& newText);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override
    {
        if (onClick)
            onClick();
    }
    std::function<void()> onClick; // l'afficheur du preset ouvre son menu au clic

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
