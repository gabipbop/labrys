#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Habillage du panneau : façade noire, sérigraphie ivoire, flancs cuivre, boutons ambre (un seul thème).
namespace dedale::ui
{
// Le panneau est dessiné dans ce repère fixe, puis mis à l'échelle par l'éditeur (75 % à 200 %).
inline constexpr int kDesignWidth = 960;
inline constexpr int kDesignHeight = 444;

namespace colour
{
inline const juce::Colour panelTop{0xff1b1b1c};
inline const juce::Colour panelBottom{0xff0e0e0f};
inline const juce::Colour ink{0xffefe8d8};    // sérigraphie
inline const juce::Colour sub{0xffa8a294};    // sérigraphie secondaire
inline const juce::Colour tabInk{0xff111111}; // texte sur cartouche plein
inline const juce::Colour lit{0xfff4b63a};    // bouton allumé, afficheurs
inline const juce::Colour off{0xff2a2a2c};
inline const juce::Colour offStroke{0xff55554f};
inline const juce::Colour bezel{0xff050505};
inline const juce::Colour bezelStroke{0xff5a5a55};
inline const juce::Colour glass{0xff070808};
inline const juce::Colour railDark{0xff7a3f1c};
inline const juce::Colour railLight{0xffc8733a};
inline const juce::Colour ledGreen{0xff57e06a}; // play head
inline const juce::Colour ledRed{0xffff4b4b};   // pas actif
inline const juce::Colour ledOff{0xff2b1414};
inline const juce::Colour writeHead{0xff3fd7ea};
inline const juce::Colour cableAudio{0xfff4b63a};
inline const juce::Colour cableCv{0xff6fc3ee};
inline const juce::Colour cableTrigger{0xffefe8d8};
} // namespace colour

// Barlow Condensed (OFL), embarquée dans le plugin.
juce::Font font(float height, bool bold = false, float kerning = 0.06f);

// Texte centré sur (cx, baseline approximative y).
void drawLabel(juce::Graphics& g, const juce::String& text, float cx, float y, float height,
               juce::Colour colour, bool bold = false,
               juce::Justification justification = juce::Justification::centred);

// Fenêtre vitrée (afficheur, oscilloscope).
void drawGlass(juce::Graphics& g, juce::Rectangle<float> area);
} // namespace dedale::ui
