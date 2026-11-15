#pragma once

#include "Patch/PatchBay.h"
#include "Sequencer/GenerativeSequencer.h"
#include "Settings.h"

#include <juce_data_structures/juce_data_structures.h>

// Conversion de l'état hors paramètres (séquence, câbles, réglages globaux) vers et depuis un ValueTree,
// pour la sauvegarde du projet et les presets. Lecture tolérante : ce qui manque garde sa valeur de repli.
namespace dedale::stateio
{
inline constexpr int kStateVersion = 1;
using Cables = std::array<int, kNumInputs>; // sortie reliée à chaque entrée, ou −1

extern const juce::Identifier kVersion, kSequencer, kCables, kSettings;

juce::ValueTree toTree(const SequencerState& state);
SequencerState sequencerFromTree(const juce::ValueTree& tree, SequencerState fallback);

juce::ValueTree toTree(const Cables& cables);
Cables cablesFromTree(const juce::ValueTree& tree);

juce::ValueTree toTree(const GlobalSettings& settings);
GlobalSettings settingsFromTree(const juce::ValueTree& tree, GlobalSettings fallback);
} // namespace dedale::stateio
