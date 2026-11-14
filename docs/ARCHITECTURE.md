# Architecture

> Document vivant : il décrit ce qui existe, pas ce qui est prévu. La cible est décrite dans `SPEC_DEDALE.md`.

## Cibles CMake (phase 0)
| Cible | Type | Rôle |
|---|---|---|
| `ariane` (`labrys::ariane`) | INTERFACE (STATIC plus tard) | bibliothèque partagée de la suite, vide pour l'instant |
| `ariane_warnings` | INTERFACE | options de warnings stricts pour le code LABRYS |
| `DedaleTests` | exécutable Catch2 | tests de DÉDALE, découverts par `ctest` |

## Dépendances
- Catch2 v3.8.1 (FetchContent, tests seulement).

## À venir
- Phase 1 : JUCE 8, cible plugin `Dedale` (VST3, AU, Standalone).
- Phase 2 : flux des classes du séquenceur, threads (audio / message / UI) et FIFO entre eux.
