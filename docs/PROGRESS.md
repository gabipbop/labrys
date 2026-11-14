# Avancement

## Phase 0 — Mise en place : terminée (v0.0.0, PR #1)

## Phase en cours : 1 — Squelette (branche `phase-1-skeleton`)

### Plan
- JUCE 8.0.15 (FetchContent, tag figé), cible `Dedale` (VST3, AU, Standalone), identité depuis `SuiteIdentity.cmake`.
- Processor vide (sortie stéreo silencieuse, MIDI in/out), éditeur provisoire.
- Validation : `auval`, pluginval niveaux 5 et 10, en local et en CI (niveau 5).

### Fait
- Build zéro warning sur les 4 presets ; binaires universal (arm64 + x86_64).
- `auval -v aumu Ddle Rwps` : OK. pluginval niveau 5 et 10, VST3 et AU : OK.
- Nom DAW en ASCII (« DEDALE ») après un échec de chargement du VST3 avec l'accent (ADR-007).
- Plugins rangés sous « LABRYS » dans la DAW au lieu de « Raw Powa Studio » (ADR-010, demande de Gaby).

### Reste à faire
- Gaby : DÉDALE visible dans Ableton ; vérifier après le passage sous LABRYS qu'il est rangé sous LABRYS et s'ouvre (VST3 et AU).
- CI verte, puis PR de fin de phase 1.

### Questions ouvertes
- Jacks exposés à la DAW (sorties CV, entrées sidechain), voire entre instruments : à trancher avant la phase 4.

## Prochaine phase : 2 — Séquenceur
`GenerativeSequencer` + quantizer + horloge interne/hôte, tous les tests §8.9 verts avant toute UI.
