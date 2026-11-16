# Architecture

> Document vivant : il décrit ce qui existe, pas ce qui est prévu. La cible est décrite dans `SPEC_DEDALE.md`.

## Cibles CMake
| Cible | Type | Rôle |
|---|---|---|
| `ariane` (`labrys::ariane`) | INTERFACE (STATIC plus tard) | bibliothèque partagée de la suite, vide pour l'instant |
| `ariane_warnings` | INTERFACE | options de warnings stricts pour le code LABRYS |
| `dedale_core` | STATIC, sans JUCE | séquenceur et horloges (`Source/Sequencer`), DSP (`Source/DSP`), patch bay (`Source/Patch`), moteur (`Source/Engine`) |
| `DedaleTests` | exécutable Catch2 | tests de `dedale_core`, découverts par `ctest` |
| `Dedale` (+ `Dedale_VST3`, `Dedale_AU`, `Dedale_Standalone`) | plugin JUCE | processor + éditeur provisoire (`plugins/dedale/Source`) |
| `DedaleRender` | exécutable | rend les scénarios d'écoute en WAV, réécrit les métriques de référence (`Tools/`) |
| `DedaleGoldenTests` | exécutable Catch2 | rend les scénarios par le processor et les compare à `Tests/golden/metrics.json` |

## Dépendances
- JUCE 8.0.15 (FetchContent) ; `juce_dsp` pour le suréchantillonnage seulement.
- Police Chakra Petch (licence OFL, `plugins/dedale/Resources/fonts`), embarquée par `juce_add_binary_data`.
- Catch2 v3.8.1 (FetchContent, tests seulement).
- pluginval v1.0.4 (téléchargé par `tools/pluginval.sh`).

## Flux audio
```
paramètres APVTS (atomics) ──► lissage 20 ms ─┐
transport hôte (PPQ, BPM) ──► horloge maître ─┤   DedaleProcessor::renderChunk (JUCE)
MIDI (Note On) ───────────────────────────────┘
        │ par échantillon de base                       │ ×4, bloc suréchantillonné (juce::dsp::Oversampling)
        ▼                                               ▼
Engine::processControl                           Engine::processAudio = Voice::processSample
  entrées CLOCK 1/2, RESET, BIT FLIP 1/2,          entrées 1V/OCT, SYNC, FOLD, CUTOFF, VCA CV, BLEND,
          TRIGGER, EG2 TRIG                                VCW IN, VCF IN, U MIX 1/2
  GenerativeSequencer::tick ×2                     MOD VCO ─► TZFM ─► VCO ; ring ; bruit ─► mixeur saturant
  EG TRIG MIX ─► DecayEnv ×2                       ─► ORDER : wavefolder et filtre ─► VCA ×2 ─► BLEND
  sorties EG1, EG2, SEQ CV, SEQ TRIG, CLOCK        ─► VOLUME ─► DC-block ─► écrêtage ─► ±1
  Voice::setControls (valeurs sans câble)          sorties M VCO, NOISE, MIXER, U MIX 1+2, VCA
        └──────────────── PatchBay : 12 tensions de sortie, 20 sources d'entrée ────────────────┘
```
- `PatchBay` : une entrée lit la sortie reliée, ou sa valeur de normalisation. Une sortie pas encore recalculée
  est lue avec un échantillon de retard, ce qui rend le feedback sûr (ADR-016).
- `Engine` possède le séquenceur, les enveloppes, la voix et la patch bay ; il ne connaît pas JUCE.
- `GenerativeSequencer` possède les deux `SeqLane`, le mode chain, le buffer et le RNG (`Rng`, xoshiro128**).
- Tensions en volts partout (audio ±5 V, enveloppes 0 à +8 V, triggers 0/+5 V), sortie ramenée à ±1.
- `DedaleProcessor::sequencer()` ne sert qu'au rendu hors temps réel.
- Constantes et hypothèses : `Source/Calibration.h`.

## Interface (phase 5)
```
DedaleEditor ── échelle 75–200 % ──► ui::Panel (repère fixe 960 × 444, positions dans UI/Layout.h)
  ├─ Background (façade statique, en cache)      ├─ SequencerLeds, ModeDisplay, ScopeView (30 Hz)
  ├─ Knob ×32 (SliderAttachment → APVTS)         ├─ PatchBayView (souris) + CableOverlay (dessin)
  └─ SquareButton → ButtonComboHandler ─► Action ─► DedaleProcessor::perform
                                                      ├─ réglages : paramètres APVTS
                                                      └─ séquenceur : FIFO sans verrou → thread audio
câbles : DedaleProcessor::connectCable / disconnectCable (table d'atomics) → recopiée dans PatchBay à chaque bloc
affichage : DedaleProcessor::display (atomics) et pullScope (FIFO) ← thread audio
```
- Threads : l'interface ne touche jamais au moteur. Tout passe par les paramètres, la FIFO d'actions, la table de
  câbles et l'état d'affichage (ADR-017).

## État, presets, MIDI (phase 6)
- `StateIO` convertit séquence, câbles et réglages globaux vers et depuis le `ValueTree` du projet.
  `DedaleProcessor::captureState` / `applyState` servent au projet (avec la séquence) et aux presets (sans).
- `PresetManager` (`Presets.cpp`) : presets d'usine dans le code, presets utilisateur en XML.
- `Settings.h` : réglages globaux, lus par le thread audio dans un entier atomique.
- `MidiClock` (pur) prend la main sur l'horloge hôte ou interne tant que des impulsions arrivent.
- Paramètres que le moteur peut changer lui-même (`run`, `chain`, `quant_mode`) : recalés par une minuterie du
  processor sur le thread de l'interface (ADR-018).
- `DedaleBench` : coût de `processBlock` et budget de CPU.

## À venir
- Phase 7 : calibration à l'écoute.
