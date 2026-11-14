# Architecture

> Document vivant : il décrit ce qui existe, pas ce qui est prévu. La cible est décrite dans `SPEC_DEDALE.md`.

## Cibles CMake
| Cible | Type | Rôle |
|---|---|---|
| `ariane` (`labrys::ariane`) | INTERFACE (STATIC plus tard) | bibliothèque partagée de la suite, vide pour l'instant |
| `ariane_warnings` | INTERFACE | options de warnings stricts pour le code LABRYS |
| `DedaleTests` | exécutable Catch2 | tests de DÉDALE, découverts par `ctest` |
| `Dedale` (+ `Dedale_VST3`, `Dedale_AU`, `Dedale_Standalone`) | plugin JUCE | processor vide + éditeur provisoire (`plugins/dedale/Source`) |

## Dépendances
- JUCE 8.0.15 (FetchContent).
- Catch2 v3.8.1 (FetchContent, tests seulement).
- pluginval v1.0.4 (téléchargé par `tools/pluginval.sh`).

## À venir
- Phase 2 : flux des classes du séquenceur, threads (audio / message / UI) et FIFO entre eux.
