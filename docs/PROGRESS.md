# Avancement

## Phase 0 — Mise en place : terminée (v0.0.0, PR #1)

## Phase 1 — Squelette : terminée (PR #2)
Vérifié par Gaby dans Ableton (rangé sous LABRYS), auval et pluginval 10 OK.

## Phase 2 — Séquenceur : mergée (v0.2.0, PR #3), réponses d'écoute encore attendues

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

## Phase 3 — Voix : mergée (v0.3.0, PR #4), réponses d'écoute encore attendues
Démarrée à la demande de Gaby (2026-11-15) avant ses réponses au checkpoint de la phase 2 (PR 3 mergée le même
jour à sa demande) : ses retours d'écoute seront intégrés quand ils arriveront.

### Plan
- Modules DSP purs dans `Source/DSP` (dans `dedale_core`) : `Vco` (sinus, TZFM), `ModVco` (triangle PolyBLAMP),
  `Noise`, `SaturatingMixer`, `Wavefolder`, `VcFilter` (SVF TPT), `DecayEnv`, `Voice` (câblage par défaut, ORDER,
  VCAs, BLEND, sortie). Constantes dans `Calibration.h`.
- Tests d'abord (§15.6) : `Tests/DspTests.cpp` (fréquence, 1 V/oct, −3 dB, pas d'auto-oscillation, VCA à 0 V,
  decay à −60 dB, triangle anti-aliasé, mixeur, folder).
- Processor : paramètres du §10 pour la voix, lissage ~20 ms, enveloppes à la fréquence de base, voix
  suréchantillonnée ×4 (`juce::dsp::Oversampling`). `DebugBeep` supprimé.
- Rendus `init_patch`, `tzfm_sweep`, `fold_sweep`, `fold_bias_sweep`, `vcf_res_sweep`, `mixer_drive`,
  `order_compare` ; golden tests en métriques JSON ; mesure d'aliasing dans `tools/analyse.py`.
- Checkpoint : `LISTENING_CHECKLIST.md`, spectrogrammes, PR.

### Fait
- 8 modules DSP purs + `Voice`, 11 tests DSP (fréquence et 1 V/oct à ±3 cents, TZFM, triangle anti-aliasé, bruit,
  mixeur, folder, filtre à −3 dB et sans auto-oscillation, VCA, enveloppe à −60 dB, voix complète).
- Processor : 27 paramètres de voix (§10), lissage 20 ms, enveloppes et EG TRIG MIX, suréchantillonnage ×4.
- 14 scénarios de rendu, golden tests (`DedaleGoldenTests`, métriques dans `Tests/golden/metrics.json`).
- `tools/analyse.py` : mesure d'aliasing (`--f0`), détection de clics relative au timbre. Aucun clic, aucune
  valeur non finie sur les 14 rendus.
- 40 tests verts sur les 4 presets et sous ASan/UBSan ; pluginval niveau 10 (VST3, AU) et `auval` : OK.
- Coût : environ 60 fois le temps réel en release sur ce Mac (budget de la phase 6 : 20 fois).

### En attente
- **Réponses de Gaby aux checkpoints des phases 2 et 3** (`LISTENING_CHECKLIST.md`) avant la phase 4.

### Hypothèses à valider (ADR-015)
- Modèle du wavefolder et toutes ses constantes ; loi d'EG TRIG MIX ; profondeurs de modulation d'EG1.
- DC-block avant l'écrêtage de sécurité (écart au §6). Aliasing à ×4 : −32,7 dB dans le pire cas.

### Reporté
- Drift analogique, interrupteur de coloration du VCW, oversampling réglable : phase 6.
- U MIX (`umix1_lvl`), SYNC du MOD VCO, entrées audio directes : phase 4. Bouton TRIGGER : phase 5.

## Phase 4 — Patch bay : mergée (v0.4.0, PR #5), réponses d'écoute encore attendues
Démarrée à la demande de Gaby (2026-11-15), retours d'écoute des phases 2 et 3 encore attendus.
Jacks exposés à la DAW : on suit la spec §9 (extension facultative de la phase 6), rien en phase 4.

### Plan
- `Source/Patch/PatchBay.h` : 20 entrées, 12 sorties, un câble par entrée, normalisations cassées par le câble,
  détecteur de gate à hystérésis (+1 V / +0,5 V).
- `Source/Engine.{h,cpp}` (pur) : reprend l'orchestration du processor (séquenceur, enveloppes, voix) et lit ses
  entrées dans la patch bay. Contrôle à la fréquence de base, audio suréchantillonné ; une sortie pas encore
  calculée est lue avec un échantillon de retard (feedback).
- `Voice` lit ses entrées dans la patch bay (1V/OCT, SYNC, FOLD, CUTOFF, VCA CV, BLEND, VCW IN, VCF IN, U MIX).
- Tests d'abord : `Tests/PatchTests.cpp` (routage, normalisations, M VCO → BIT FLIP à l'audio-rate, CLOCK 2
  indépendant, RESET, TRIGGER / EG2 TRIG, feedback stable). Les golden de la phase 3 ne doivent pas bouger.
- Paramètre `umix1_lvl`. Rendus `patch_*`, checklist, PR.

### Fait
- `PatchBay` (20 entrées, 12 sorties), `GateDetector`, `Engine` ; `Voice` lit ses 12 entrées dans la patch bay.
- 10 tests de patch : câbles et normalisations, hystérésis, M VCO → BIT FLIP 1 à l'audio-rate (environ 500 flips
  par seconde, sans crash), CLOCK 2 indépendant (4 contre 3 pas par seconde), CLOCK 1 qui entraîne CLOCK 2, RESET,
  TRIGGER et EG2 TRIG, neuf câbles de feedback simultanés (sortie finie et bornée), chaque entrée de la voix.
- Les 14 golden de la phase 3 passent sans changement après le déplacement de l'orchestration dans `Engine`.
- Paramètre `umix1_lvl`. 6 rendus `patch_*`, ajoutés aux golden (20 scénarios).
- 50 tests verts sur les 4 presets et sous ASan/UBSan ; pluginval niveau 10 (VST3, AU) et `auval` : OK.
- Coût inchangé : environ 58 fois le temps réel en release.

### En attente
- **Réponses de Gaby aux checkpoints des phases 2, 3 et 4** (`LISTENING_CHECKLIST.md`).

### Hypothèses à valider (ADR-016)
- EG2 TRIG patché isole EG2 ; échelle des CV externes (±5 V = toute la course) ; U MIX saturé à ±10 V.

### Reporté
- Câbles dans le state, câblage depuis l'UI pendant la lecture : phases 5 et 6. Jacks exposés à la DAW : phase 6.

## Phase en cours : 5 — UI (branche `phase-5-ui`)
Direction validée par Gaby (2026-11-15) après deux séries de maquettes : rendu « synthé matériel classique »
(esprit Mariana), façade noire à modules encadrés, knobs à jupe noire et capuchon métal, flancs cuivre.
**Un seul thème, le noir** (décision de Gaby : pas de thème clair, contrairement au §11.2 de la spec).
Pas de logo, nom, police ni bois Moog (règle du projet).

### Retours de Gaby à traiter (2026-11-15, pendant le codage)
- **Typo** : il n'aime pas du tout Barlow Condensed. À remplacer.
- **LEDs** : « affreuses ». À redessiner.
- Direction générale : plus **agressif / industriel**. Le rendu actuel fait « trop gentil et trop IA ».
- Proposé le même jour : trois habillages rendus par le plugin (`docs/checkpoints/phase-5/panneau_1..3.png`,
  `panneau_0.png` = première version), choisis par `ui::setSkin` : **1 Acier** (Saira Condensed + pochoir,
  fentes lumineuses, rouge signal, pans coupés, vis), **2 Terminal** (Share Tech Mono, voyants carrés, blanc et
  rouge), **3 Chantier** (Chakra Petch, pastilles à pans coupés, jaune, flancs à bandes). **En attente du choix
  de Gaby** ; les habillages non retenus et leurs polices seront retirés.

### Plan
- `Sequencer/ButtonComboHandler` (pur, tests d'abord) : combinaisons du §8.4, appui long de BUFFER, bouton SHIFT
  qui verrouille les appuis pour les combinaisons à la souris.
- Processor : FIFO d'actions UI → audio, câbles et état d'affichage en atomics, tampon d'oscilloscope.
- `Source/UI` : thème noir, police Barlow Condensed (OFL) en BinaryData, knobs et boutons vectoriels,
  LEDs du séquenceur, afficheur du quantizer, sélecteur ORDER, oscilloscope, patch bay avec câbles à glisser.
- Éditeur redimensionnable de 75 % à 200 %, infobulles, valeur affichée pendant le réglage.
- Vérification : captures PNG du panneau par `DedaleRender --ui`, pluginval, auval.

### Fait
- `ButtonComboHandler` et ses tests (combinaisons du §8.4, appui long de BUFFER).
- Pont interface ↔ audio dans le processor (FIFO d'actions, table de câbles, état d'affichage, oscilloscope).
- Panneau complet (`Source/UI`) : 32 knobs, boutons, LEDs, afficheur du quantizer, ORDER, oscilloscope, patch
  bay avec câbles, infobulles, redimensionnement. Captures : `docs/checkpoints/phase-5/`.
- 54 tests verts sur les 4 presets et sous ASan ; pluginval niveau 10 **avec** ses tests d'éditeur et `auval` : OK.

### Reste à faire
- **Essai par Gaby dans la DAW** : les interactions à la souris (knobs, boutons, combinaisons, câbles) ne sont
  pas testées automatiquement.
- Retours de Gaby ci-dessus : typo et LEDs, direction plus agressive et industrielle.
- Checklist de la phase, PR.
