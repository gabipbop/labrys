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
inline const juce::Colour tabInk{0xff111111}; // texte sur cartouche plein
inline const juce::Colour off{0xff2a2a2c};
inline const juce::Colour offStroke{0xff55554f};
inline const juce::Colour bezel{0xff050505};
inline const juce::Colour bezelStroke{0xff5a5a55};
inline const juce::Colour glass{0xff070808};
inline const juce::Colour ledGreen{0xff57e06a}; // play head
inline const juce::Colour ledRed{0xffff4b4b};   // pas actif
inline const juce::Colour ledOff{0xff2b1414};
inline const juce::Colour writeHead{0xff3fd7ea};
inline const juce::Colour cableAudio{0xfff4b63a};
inline const juce::Colour cableCv{0xff6fc3ee};
inline const juce::Colour cableTrigger{0xffefe8d8};
} // namespace colour

// Forme des LEDs du séquenceur.
enum class LedShape
{
    round, // LED ronde à collerette
    slit,  // fente lumineuse rectangulaire
    lamp,  // voyant carré à cadre métal
    cut    // pastille à pans coupés
};

// Habillage : tout ce qui change d'une proposition à l'autre. Un seul sera gardé.
struct Skin
{
    const char* name;
    int fontFamily; // 0 Barlow Condensed, 1 Saira Condensed + stencil, 2 Share Tech Mono + Quantico, 3 Chakra
                    // Petch
    float fontScale;                  // pour garder le même encombrement d'une police à l'autre
    juce::Colour ink;                 // sérigraphie
    juce::Colour sub;                 // sérigraphie secondaire
    juce::Colour lit;                 // boutons allumés, afficheurs
    juce::Colour railDark, railLight; // flancs
    LedShape led;
    bool chamfer; // cadres et boutons à pans coupés au lieu d'arrondis
    bool bolts;   // vis apparentes aux coins des modules
    bool hazard;  // flancs à bandes de signalisation
};
const Skin& skin() noexcept;
void setSkin(int index) noexcept;
inline constexpr int kNumSkins = 4;

juce::Font font(float height, bool bold = false, float kerning = 0.06f);
juce::Font titleFont(float height);

// Rectangle à pans coupés (ou arrondi, selon l'habillage).
juce::Path framePath(juce::Rectangle<float> area, float corner);
// Texte centré sur (cx, baseline approximative y).
void drawLabel(juce::Graphics& g, const juce::String& text, float cx, float y, float height,
               juce::Colour colour, bool bold = false,
               juce::Justification justification = juce::Justification::centred);

// Fenêtre vitrée (afficheur, oscilloscope).
void drawGlass(juce::Graphics& g, juce::Rectangle<float> area);
} // namespace dedale::ui
