# Avancement

## Phase 0 — Mise en place : terminée (v0.0.0, PR #1)

## Phase 1 — Squelette : terminée (PR #2)
Vérifié par Gaby dans Ableton (rangé sous LABRYS), auval et pluginval 10 OK.

## Phase en cours : 2 — Séquenceur (branche `phase-2-sequencer`)

### Plan
- Bibliothèque pure `dedale_core` (aucun JUCE) : `Sequencer/{Rng,Quantizer,GenerativeSequencer,Clock}`, `Calibration.h`.
- Tests d'abord (§8.9) : `Tests/{Quantizer,Sequencer,Clock}Tests.cpp` remplacent le test smoke.
- Processor : APVTS (tempo, sync_mode, clock_div, seqN_corrupt, seqN_range, run, chain, quant_mode),
  horloge interne/hôte sample-accurate, bip sinus de debug par trigger (`DSP/DebugBeep.h`), Note On MIDI.
- Outils : cible `DedaleRender` (WAV 48 kHz/24 bits dans `renders/`), `tools/analyse.py`.
- Checkpoint : rendus `seq_*.wav`, spectrogrammes, questions dans `LISTENING_CHECKLIST.md`, PR.

### Fait
- `GenerativeSequencer`, `Quantizer`, `InternalClock`, `HostClock` : 28 tests (dont tous ceux du §8.9), verts sur
  les 4 presets et sous ASan/UBSan.
- Processor : 10 paramètres APVTS, horloge hôte sample-accurate ou interne, bip de debug, Note On (transposition,
  trigger à l'arrêt), state = paramètres.
- `DedaleRender` : 6 rendus `seq_*.wav`. `tools/analyse.py` : aucune valeur non finie, aucun clic, attaques sur la
  grille des pas, nombre d'attaques conforme aux motifs (40 pour `seq_corrupt_0`, soit 5 pas actifs × 8 boucles).
- pluginval niveau 10 (VST3 et AU) et `auval` : OK.

### En attente
- **Réponses de Gaby à `LISTENING_CHECKLIST.md`** (checkpoint) avant de démarrer la phase 3.

### Hypothèses à valider (détail dans ADR-012 et ADR-013)
- CORRUPT avant la lecture du pas ; RESET : le prochain tick joue le pas 1 ; ADVANCE joue le pas sans CORRUPT.
- Polarité unipolaire appliquée avant le quantizer (écart à la spec §8.2, pour rester dans la gamme).
- Division hôte : N doubles croches par pas ; transport arrêté : horloge interne.
- Gammes 7, 9 et 15 du quantizer.

### Reporté
- MIDI clock, Start/Stop/Continue, canal MIDI, MIDI out, séquence dans le state : phase 6.
- Jacks CLOCK 1/2, RESET, BIT FLIP : phase 4. Combinaisons de boutons et LEDs : phase 5. Golden tests : phase 3.

### Questions ouvertes
- Jacks exposés à la DAW (sorties CV, entrées sidechain), voire entre instruments : à trancher avant la phase 4.

## Prochaine phase : 3 — Voix
VCO + MOD VCO + TZFM + ring + noise + mixeur + VCW + VCF + VCAs + EGs + BLEND + ORDER, normalisations en dur.
