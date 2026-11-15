# Changelog

Format : [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/), versions sémantiques.

## [Non publié]
### Ajouté
- Plugin DÉDALE vide (VST3, AU, Standalone) sur JUCE 8.0.15, validé par auval et pluginval (niveau 10).
- `tools/pluginval.sh` ; la CI lance pluginval niveau 5 et auval.

## [0.0.0] — 2026-11-15
### Ajouté
- Mise en place du monorepo LABRYS : bibliothèque ARIANE, dossier DÉDALE, identité de la suite centralisée.
- Build CMake avec presets `debug`, `release` (universal), `tests`, `asan`, tests Catch2.
- CI GitHub Actions (build release universal + tests + vérification du formatage).
