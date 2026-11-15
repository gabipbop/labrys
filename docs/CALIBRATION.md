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
| `kBeep*` | 261,63 Hz, 0,12 s, 0,3, pan 0,7 | Bip de debug, supprimé en phase 3 | — | |

Gammes du quantizer à vérifier (`// TODO calibrate` dans `Quantizer.cpp`) : 7 (diminished 6th), 9 (hirajoshi), 15 (handpan).
