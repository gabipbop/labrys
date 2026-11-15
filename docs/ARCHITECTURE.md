# Architecture

> Document vivant : il décrit ce qui existe, pas ce qui est prévu. La cible est décrite dans `SPEC_DEDALE.md`.

## Cibles CMake
| Cible | Type | Rôle |
|---|---|---|
| `ariane` (`labrys::ariane`) | INTERFACE (STATIC plus tard) | bibliothèque partagée de la suite, vide pour l'instant |
| `ariane_warnings` | INTERFACE | options de warnings stricts pour le code LABRYS |
| `dedale_core` | STATIC, sans JUCE | séquenceur, quantizer, horloges (`Source/Sequencer`) |
| `DedaleTests` | exécutable Catch2 | tests de `dedale_core`, découverts par `ctest` |
| `Dedale` (+ `Dedale_VST3`, `Dedale_AU`, `Dedale_Standalone`) | plugin JUCE | processor + éditeur provisoire (`plugins/dedale/Source`) |
| `DedaleRender` | exécutable | rend les scénarios d'écoute en WAV (`Tools/DedaleRender.cpp`) |

## Dépendances
- JUCE 8.0.15 (FetchContent).
- Catch2 v3.8.1 (FetchContent, tests seulement).
- pluginval v1.0.4 (téléchargé par `tools/pluginval.sh`).

## Séquenceur (phase 2)
```
paramètres APVTS (atomics) ──┐
transport hôte (PPQ, BPM) ───┤          par échantillon, thread audio
MIDI (Note On) ──────────────┤
                             ▼
DedaleProcessor::processBlock
  ├─ HostClock (PPQ → ticks) ou InternalClock (tempo → ticks)
  ├─ GenerativeSequencer::tick(lane) ×2 → trigger ; cv(lane) = tension tenue × CV RANGE → Quantizer
  └─ DebugBeep ×2 (provisoire) → sortie stéréo
```
- `GenerativeSequencer` possède les deux `SeqLane`, le mode chain, le buffer et le RNG (`Rng`, xoshiro128**).
  Une méthode par action du §8.3/§8.4 ; chaque lane a sa propre entrée d'horloge (`tick`).
- Threads : tout le séquenceur tourne sur le thread audio. L'UI n'y touche pas encore ; les actions de boutons et
  l'état des LEDs passeront par une FIFO lock-free en phase 5. `DedaleProcessor::sequencer()` ne sert qu'au rendu
  hors temps réel.
- Constantes et hypothèses : `Source/Calibration.h`.

## À venir
- Phase 3 : la voix remplace `DebugBeep`.
