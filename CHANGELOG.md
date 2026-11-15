# Changelog

Format : [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/), versions sémantiques.

## [Non publié]
### Ajouté
- Plugin DÉDALE vide (VST3, AU, Standalone) sur JUCE 8.0.15, validé par auval et pluginval (niveau 10).
- `tools/pluginval.sh` ; la CI lance pluginval niveau 5 et auval.
- Séquenceur génératif double : deux lanes de 8 pas, CORRUPT, BIT FLIP, BIT SHIFT, LENGTH, CHAIN (16 pas), buffer.
- Quantizer à 16 modes, transposable par Note On MIDI.
- Horloge interne (10 à 600 pas/min) et synchronisation sur le transport de la DAW, à l'échantillon près.
- Paramètres : `tempo`, `sync_mode`, `clock_div`, `seq1_corrupt`, `seq2_corrupt`, `seq1_range`, `seq2_range`, `run`,
  `chain`, `quant_mode`. Bip sinus de debug à chaque trigger (provisoire).
- Outils : `DedaleRender` (rendus WAV hors temps réel) et `tools/analyse.py` (mesures et spectrogrammes).

## [0.0.0] — 2026-11-15
### Ajouté
- Mise en place du monorepo LABRYS : bibliothèque ARIANE, dossier DÉDALE, identité de la suite centralisée.
- Build CMake avec presets `debug`, `release` (universal), `tests`, `asan`, tests Catch2.
- CI GitHub Actions (build release universal + tests + vérification du formatage).
