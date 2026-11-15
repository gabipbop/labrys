#pragma once

#include "../Patch/PatchBay.h"
#include "../Sequencer/ButtonComboHandler.h"

#include <array>

// Positions de tous les éléments du panneau, dans le repère de dessin (960 × 444).
namespace dedale::ui::layout
{
struct Module
{
    float x, y, w, h;
    const char* name;
};
inline constexpr std::array<Module, 8> kModules{{
    {28, 52, 476, 164, "SÉQUENCEUR"},
    {512, 52, 244, 164, "ENVELOPPES"},
    {28, 230, 188, 196, "SOURCES"},
    {224, 230, 104, 196, "MIXEUR"},
    {336, 230, 168, 196, "WAVEFOLDER"},
    {512, 230, 76, 196, "ROUTAGE"},
    {596, 230, 160, 196, "FILTRE"},
    {764, 52, 168, 374, "PATCH BAY"},
}};

struct Knob
{
    const char* parameter;
    float x, y, radius; // radius < 9 : petit trimmer sans graduations
    const char* label;
    const char* tooltip;
};
inline constexpr std::array<Knob, 32> kKnobs{{
    {"tempo", 56, 146, 11.5f, "TEMPO", "Tempo de l'horloge interne, en pas par minute."},
    {"seq1_corrupt", 418, 101, 10.5f, "CORRUPT",
     "Mutation de SEQ1 : les notes changent jusqu'à midi, le rythme aussi au-delà."},
    {"seq1_range", 470, 101, 10.5f, "CV RANGE", "Étendue des notes de SEQ1, d'une octave à dix."},
    {"seq2_corrupt", 418, 193, 10.5f, "CORRUPT",
     "Mutation de SEQ2 : les notes changent jusqu'à midi, le rythme aussi au-delà."},
    {"seq2_range", 470, 193, 10.5f, "CV RANGE", "Étendue des notes de SEQ2, d'une octave à dix."},
    {"eg1_decay", 554, 110, 16, "EG1 DECAY", "Durée de l'enveloppe 1, qui module hauteurs, FOLD et CUTOFF."},
    {"eg2_decay", 616, 110, 16, "EG2 DECAY", "Durée de l'enveloppe 2, qui ouvre les deux VCA."},
    {"eg_trig_mix", 678, 110, 16, "EG TRIG MIX",
     "Qui déclenche les enveloppes : SEQ1 à gauche, SEQ2 à droite, les deux au milieu."},
    {"volume", 730, 112, 12, "VOLUME", "Niveau de sortie."},
    {"vco_freq", 74, 294, 24, "VCO FREQUENCY", "Hauteur du VCO sinus."},
    {"vco_eg1_amt", 134, 298, 8, "EG1 AMT", "Modulation de la hauteur du VCO par l'enveloppe 1 (bipolaire)."},
    {"vco_seq1_amt", 176, 298, 8, "SEQ1 AMT",
     "SEQ1 vers la hauteur du VCO. En butée : notes justes et quantifiées."},
    {"mvco_freq", 74, 380, 22, "MOD VCO FREQUENCY", "Hauteur du MOD VCO triangle, du LFO à l'audio."},
    {"mvco_eg1_amt", 128, 386, 8, "EG1",
     "Modulation de la hauteur du MOD VCO par l'enveloppe 1 (bipolaire)."},
    {"mvco_seq2_amt", 160, 386, 8, "SEQ2",
     "SEQ2 vers la hauteur du MOD VCO. En butée : notes justes et quantifiées."},
    {"mvco_fm_amt", 192, 386, 8, "FM", "FM through-zero du VCO par le MOD VCO."},
    {"mix_vco", 252, 284, 11, "VCO", "Niveau du VCO. Gain unité à midi, saturation au-delà."},
    {"mix_ring", 300, 284, 11, "RING MOD", "Niveau du ring mod (VCO × MOD VCO)."},
    {"mix_mvco", 252, 342, 11, "MOD VCO", "Niveau du MOD VCO."},
    {"mix_noise", 300, 342, 11, "NOISE", "Niveau du bruit."},
    {"noise_tone", 254, 400, 11, "NOISE TONE", "Couleur du bruit, du grave à l'aigu."},
    {"umix1_lvl", 304, 404, 8, "U MIX 1", "Niveau du canal 1 du mixeur utilitaire (ring mod par défaut)."},
    {"vcw_fold", 388, 316, 27, "FOLD", "Quantité de pli du wavefolder."},
    {"vcw_bias", 462, 288, 11.5f, "BIAS", "Décale la symétrie du pli : harmoniques paires ou impaires."},
    {"vcw_eg1_amt", 444, 374, 8, "EG1 / CV", "Enveloppe 1 (ou jack FOLD) vers le pli, bipolaire."},
    {"vcw_seq1_amt", 480, 374, 8, "SEQ1", "SEQ1 vers le pli."},
    {"blend", 550, 378, 15, "BLEND", "Mélange entre le chemin wavefolder et le chemin filtre."},
    {"vcf_cutoff", 642, 316, 27, "CUTOFF", "Fréquence de coupure du filtre."},
    {"vcf_res", 702, 288, 11.5f, "RESONANCE", "Résonance du filtre, sans auto-oscillation."},
    {"vcf_mode", 738, 288, 11.5f, "LP > BP", "Morph continu du passe-bas vers le passe-bande."},
    {"vcf_eg1_amt", 702, 374, 8, "EG1 / CV", "Enveloppe 1 (ou jack CUTOFF) vers la coupure, bipolaire."},
    {"vcf_seq2_amt", 738, 374, 8, "SEQ2", "SEQ2 vers la coupure du filtre."},
}};

// Rangées du séquenceur.
inline constexpr std::array<float, 2> kSeqRowY{98, 190};
inline constexpr float kLedX0 = 88, kLedSpacing = 24;
inline constexpr float kTransportY = 147;

struct ComboButton
{
    PanelButton button;
    float x, y;
    const char* label;
    const char* tooltip;
};
inline constexpr std::array<ComboButton, 10> kComboButtons{{
    {PanelButton::bitFlip1, 296, 99, "BIT FLIP",
     "Inverse le pas sous le write head de SEQ1. Tenu : inverse chaque pas au passage."},
    {PanelButton::bitShift1, 330, 99, "BIT SHIFT",
     "Décale la séquence SEQ1 d'un pas vers la droite. Sert aussi de touche de combinaison."},
    {PanelButton::length1, 364, 99, "LENGTH", "Raccourcit SEQ1 d'un pas (8, 7 ... 1, puis 8)."},
    {PanelButton::bitFlip2, 296, 191, "BIT FLIP",
     "Inverse le pas sous le write head de SEQ2. Tenu : inverse chaque pas au passage."},
    {PanelButton::bitShift2, 330, 191, "BIT SHIFT",
     "Décale la séquence SEQ2 d'un pas vers la droite. En mode chain : avance son play head."},
    {PanelButton::length2, 364, 191, "LENGTH", "Raccourcit SEQ2 d'un pas (8, 7 ... 1, puis 8)."},
    {PanelButton::reset, 150, 147, "RESET", "Retour au pas 1. L'offset du write head est conservé."},
    {PanelButton::advance, 184, 147, "ADVANCE", "Avance d'un pas, à l'arrêt seulement."},
    {PanelButton::buffer, 218, 147, "BUFFER",
     "Appui court : rappelle la séquence sauvegardée. Tenu 1 s : sauvegarde."},
    {PanelButton::trigger, 286, 147, "TRIGGER", "Déclenche les deux enveloppes."},
}};
inline constexpr float kButtonSize = 13;

// Prises de la patch bay : 4 colonnes, 5 rangées d'entrées puis 3 rangées de sorties.
inline constexpr float kJackX0 = 788, kJackDx = 40, kJackY0 = 92, kJackDy = 42.5f, kJackRadius = 8.6f;
inline constexpr std::array<const char*, kNumInputs> kInputNames{
    "VCO 1V/OCT", "M VCO SYNC", "M VCO 1V",   "BLEND",   "VCW IN",  "FOLD",     "VCW VCA",
    "VCF IN",     "CUTOFF",     "VCF VCA",    "U MIX 1", "U MIX 2", "EG2 TRIG", "CLOCK 1",
    "BIT FLIP 1", "CLOCK 2",    "BIT FLIP 2", "MIDI",    "TRIGGER", "RESET"};
inline constexpr std::array<const char*, kNumOutputs> kOutputNames{
    "VCA",       "M VCO",   "NOISE",     "MIXER",   "EG1",       "EG2",
    "U MIX 1+2", "SEQ1 CV", "SEQ1 TRIG", "SEQ2 CV", "SEQ2 TRIG", "CLOCK"};

constexpr float jackX(int index) noexcept
{
    return kJackX0 + static_cast<float>(index % 4) * kJackDx;
}
constexpr float jackY(bool isOutput, int index) noexcept
{
    return kJackY0 + static_cast<float>(index / 4 + (isOutput ? 5 : 0)) * kJackDy;
}
} // namespace dedale::ui::layout
