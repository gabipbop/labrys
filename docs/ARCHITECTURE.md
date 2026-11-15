# Architecture

> Document vivant : il décrit ce qui existe, pas ce qui est prévu. La cible est décrite dans `SPEC_DEDALE.md`.

## Cibles CMake
| Cible | Type | Rôle |
|---|---|---|
| `ariane` (`labrys::ariane`) | INTERFACE (STATIC plus tard) | bibliothèque partagée de la suite, vide pour l'instant |
| `ariane_warnings` | INTERFACE | options de warnings stricts pour le code LABRYS |
| `dedale_core` | STATIC, sans JUCE | séquenceur, quantizer, horloges (`Source/Sequencer`), DSP de la voix (`Source/DSP`) |
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
transport hôte (PPQ, BPM) ────────────────────┤      fréquence de base, par échantillon, thread audio
MIDI (Note On) ───────────────────────────────┤
                                              ▼
DedaleProcessor::renderChunk
  ├─ HostClock ou InternalClock → ticks
  ├─ GenerativeSequencer::tick(lane) ×2 → triggers ; cv(lane) → SEQ1 CV, SEQ2 CV
  ├─ EG TRIG MIX → DecayEnv ×2 (EG1, EG2)
  ├─ Voice::setControls(réglages, {SEQ1, SEQ2, EG1, EG2})     hauteurs, cutoff, gains
  └─ ×4 : Voice::processSample()                              bloc suréchantillonné (juce::dsp::Oversampling)
         MOD VCO ─► TZFM ─► VCO ; ring ; bruit ─► mixeur saturant
         ─► ORDER : wavefolder et filtre, en parallèle ou en série
         ─► VCA ×2 (EG2) ─► BLEND ─► VOLUME ─► DC-block ─► écrêtage de sécurité ─► ±1
```
- `GenerativeSequencer` possède les deux `SeqLane`, le mode chain, le buffer et le RNG (`Rng`, xoshiro128**).
  Une méthode par action du §8.3/§8.4 ; chaque lane a sa propre entrée d'horloge (`tick`).
- `Voice` câble en dur les normalisations par défaut (SEQ1 → VCO et FOLD, SEQ2 → MOD VCO et CUTOFF, EG1 →
  hauteurs, FOLD, CUTOFF, EG2 → VCA). La patch bay (phase 4) remplacera ce câblage.
- Tensions en volts partout (audio ±5 V, enveloppes 0 à +8 V), sortie ramenée à ±1.
- Threads : tout tourne sur le thread audio. L'UI n'y touche pas encore ; les actions de boutons et l'état des
  LEDs passeront par une FIFO lock-free en phase 5. `DedaleProcessor::sequencer()` ne sert qu'au rendu hors temps réel.
- Constantes et hypothèses : `Source/Calibration.h`.

## À venir
- Phase 4 : patch bay (routeur par échantillon) à la place du câblage en dur de `Voice`.
