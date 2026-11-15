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
- Threads : tout tourne sur le thread audio. L'UI n'y touche pas encore ; boutons, câbles et LEDs passeront par une
  FIFO lock-free en phase 5. `DedaleProcessor::sequencer()` et `patchBay()` ne servent qu'au rendu hors temps réel.
- Constantes et hypothèses : `Source/Calibration.h`.

## À venir
- Phase 5 : panneau, câbles à l'écran, FIFO UI ↔ audio.
