# Avancement

## Phase en cours : 0 — Mise en place (branche `phase-0-setup`)

### Fait
- Manuels de référence dans `docs/reference/` (gitignoré) avec texte extrait : Labyrinth, DFAM, Subharmonicon,
  Grandmother v2, Mavis v2, quickstart Mavis (images seulement).
  Manquent : patch book DFAM & Subharmonicon, PDF des réglages globaux avancés du Grandmother, guide « Patching with Intention » du Mavis.
- Dépôt git, monorepo (`libs/ariane`, `plugins/dedale`), `SuiteIdentity.cmake`.
- CMake + presets `debug`, `release`, `tests`, `asan`, Catch2 v3.8.1, test smoke : les 4 presets passent en local.
- `CLAUDE.md`, `README.md`, `CHANGELOG.md`, `.clang-format`, docs de suivi, CI GitHub Actions.

### Reste à faire
- Créer le dépôt GitHub et pousser (décision de Gaby : nom, public/privé), puis vérifier la CI verte.
- PR de fin de phase 0.

### Questions ouvertes
- Patch points accessibles depuis la DAW, voire entre instruments de la suite (question de Gaby, 2026-11-15) :
  à trancher avant la phase 4 (patch bay). Voir la proposition dans le fil de discussion.

## Prochaine phase : 1 — Squelette
Projet CMake JUCE 8 (tag figé), plugin vide VST3/AU/Standalone qui se charge dans Ableton/Reaper, passe `auval`
et `pluginval` niveau 5.
