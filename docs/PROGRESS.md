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

### Questions ouvertes
- Jacks exposés à la DAW (sorties CV, entrées sidechain), voire entre instruments : à trancher avant la phase 4.
