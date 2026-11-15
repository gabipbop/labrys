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
  `chain`, `quant_mode`.
- Outils : `DedaleRender` (rendus WAV hors temps réel) et `tools/analyse.py` (mesures et spectrogrammes).
- Voix complète : VCO sinus à FM through-zero, MOD VCO triangle, ring mod, bruit à tonalité variable, mixeur
  saturant, wavefolder, filtre SVF passe-bas ↔ passe-bande, deux VCA, deux enveloppes decay, ORDER, BLEND.
  Le patch par défaut sonne sans câble. Suréchantillonnage ×4.
- 27 paramètres de voix (`vco_freq` … `volume`, `order`), lissés sur 20 ms.
- Golden tests : 14 rendus de référence comparés en métriques. Mesure d'aliasing dans `tools/analyse.py`.
- Patch bay de 32 points : 20 entrées, 12 sorties, normalisations cassées par un câble, modulation à l'audio-rate,
  boucles de feedback. Entrées trigger avec hystérésis. Mixeur utilitaire U MIX (`umix1_lvl`).
  Pas encore de câbles à l'écran : le câblage se fait par le code en attendant le panneau.
### Retiré
- Bip sinus de debug de la phase 2.

## [0.0.0] — 2026-11-15
### Ajouté
- Mise en place du monorepo LABRYS : bibliothèque ARIANE, dossier DÉDALE, identité de la suite centralisée.
- Build CMake avec presets `debug`, `release` (universal), `tests`, `asan`, tests Catch2.
- CI GitHub Actions (build release universal + tests + vérification du formatage).
