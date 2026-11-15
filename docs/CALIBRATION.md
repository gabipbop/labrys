# Calibration

Chaque constante de `Calibration.h` : valeur, pourquoi, comment l'écouter. Les retours d'écoute de Gaby sont notés ici.

| Constante | Valeur | Pourquoi | Comment l'écouter | Retour de Gaby |
|---|---|---|---|---|
| `kCorruptVoltProbAtNoon` | 0,25 | Manuel : environ 25 % à midi | `seq_corrupt_40.wav` : les notes changent, le rythme reste | |
| `kCorruptVoltProbAtMax` | 0,5 | Manuel : environ 50 % en butée | `seq_corrupt_75.wav` | |
| `kCorruptFlipProbAtMax` | 0,5 | Manuel : bit flip de 0 % (midi) à près de 50 % (butée) | `seq_corrupt_75.wav` : le rythme mute aussi | |
| `kCorruptTiming` | `beforeRead` | Hypothèse §14 : une mutation s'entend dès le pas courant | Comparer avec `afterRead` (mutation entendue un tour plus tard) | |
| `kCorruptChangesOffBitVoltages` | `false` | Manuel : seuls les pas actifs changent de note | Inaudible directement (un pas éteint ne sort pas sa tension) | |
| `kCvRangeMinScale` | 0,1 | §8.2 : CV RANGE de 1 à 10 octaves | Paramètre `seq1_range` au minimum : la mélodie tient dans une octave | |
| `kDefaultQuantizerMode` | 2 (n°3) | Manuel : majeur par défaut | `seq_quant_modes.wav` | |
| `kQuantizerModeAfterClear` | majeur | Hypothèse (ADR-012) | — | |
| `kMidiRootNote` | 60 | §8.7 : note − 60 = transposition | Jouer un mi 4 : la séquence monte d'une tierce majeure | |
| `kTempoMin/Max/DefaultStepsPerMinute` | 10 / 600 / 120 | §8.6, plage proposée (hypothèse §14) | Paramètre `tempo` aux deux butées | |
| `kHostQuartersPerStepAtDivision1` | 0,25 | Hypothèse (ADR-013) : division 1 = double croche | Dans la DAW, `clock_div` 1, 2, 4 contre le métronome | |
| `kQtzZoneStart` | 0,97 | §3.1 : derniers 3 % de course = gain 1 | `vco_seq1_amt` à 0,96 puis 1 : les notes deviennent justes | |
| `kEgPitchOctavesPerVolt` | 0,5 | EG1 AMT à fond = 4 octaves de chute | `vco_eg1_amt` à 0,3 : attaque « zap » | |
| `kTzfmMaxIndex` | 4 | Profondeur de TZFM à fond | `tzfm_sweep.wav` : le timbre s'enrichit sans que la note bouge | |
| `kNoiseMinHz` / `kNoiseMaxHz` / `kNoiseQ` / `kNoiseRmsVolts` | 100 / 12 000 / 0,7 / 1,7 | §3.4 : bruit jamais blanc, du grave à l'aigu | `mix_noise` à 0,5, balayer `noise_tone` | |
| `kMixerMaxGain` | 5 | §3.5 : gain 4 à 6 en butée | `mixer_drive.wav` | |
| `kMixerChannelCeiling` / `kMixerSumCeiling` | 1,5 / 2 | Saturation par canal puis de la somme | `mixer_drive.wav` : de propre à « crade » | |
| `kMixerAsymmetry` | 0,1 | Harmoniques paires | `mixer_drive.wav`, fin du rendu | |
| `kFolderModel` | `softTwoStage` | §4 : variantes à comparer | Passer à `sine`, re-rendre `fold_sweep.wav` | |
| `kVcwInputGain` / `kVcwFoldGain` / `kVcwStage2Gain` | 0,7 / 9 / 1,5 | Pré-gain de 0,7 à 9,7, second étage plus poussé | `fold_sweep.wav` | |
| `kVcwCornerSharpness` | 0,97 | Coins de pli adoucis (1 = vifs) | `fold_sweep.wav` : dureté du haut du spectre | |
| `kVcwBiasRange` / `kVcwAsymmetry` | 1 / 0,05 | BIAS déplace la symétrie ; légère asymétrie propre au circuit | `fold_bias_sweep.wav` | |
| `kVcwZeroColour` / `kVcwZeroColourDrive` | oui / 0,4 | §4 : coloration résiduelle à FOLD = 0 | Début de `fold_sweep.wav` | |
| `kVcwOutputDrive` | 0,8 | Saturation finale douce | `fold_sweep.wav` | |
| `kVcfMinDamping` | 0,12 | Q ≈ 8 à fond : presque l'auto-oscillation | Fin de `vcf_res_sweep.wav` | |
| `kVcfStateCeilingVolts` | 12 | Grain du filtre quand le mixeur pousse | Fin de `mixer_drive.wav` | |
| `kCutoffOctavesPerVolt` | 1 | §5 : 1 V/octave approximatif | `vcf_eg1_amt` | |
| `kVcaExponent` | 1,5 | §6 : réponse légèrement exponentielle | Queue des notes de `init_patch.wav` | |
| `kEgAttackSeconds` / `kEgSmoothingHz` | 0,7 ms / 2 kHz | §7 : anti-clic | Attaque des notes : percussive sans clic ? | |
| `kOutputHeadroom` / `kOutputClipKneeVolts` | 0,7 / 3,5 V | Le patch init reste sous l'écrêtage de sécurité | Niveau général dans la DAW | |
| `kOversamplingStages` | 2 (×4) | §4 : ×4 minimum | `alias_fold_high.wav` (−32,7 dB non harmonique) | |
| `kParamSmoothingSeconds` | 20 ms | §10 | Tourner un paramètre vite : pas de zipper | |

Gammes du quantizer à vérifier (`// TODO calibrate` dans `Quantizer.cpp`) : 7 (diminished 6th), 9 (hirajoshi), 15 (handpan).
