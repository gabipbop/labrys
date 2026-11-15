#pragma once

// Toutes les constantes à calibrer à l'oreille ou qui portent une hypothèse (§14).
// Chaque constante est documentée dans docs/CALIBRATION.md.
namespace dedale::calibration
{
// --- CORRUPT (§8.2) ---
inline constexpr float kCorruptVoltProbAtNoon = 0.25f; // probabilité de nouvelle tension à midi
inline constexpr float kCorruptVoltProbAtMax = 0.5f;   // idem en butée
inline constexpr float kCorruptFlipProbAtMax = 0.5f;   // probabilité de bit flip en butée (0 jusqu'à midi)

// Hypothèse : CORRUPT agit avant la lecture du pas, pour qu'une mutation s'entende tout de suite.
enum class CorruptTiming
{
    beforeRead,
    afterRead
};
inline constexpr CorruptTiming kCorruptTiming = CorruptTiming::beforeRead;

// Hypothèse : seuls les pas actifs reçoivent une nouvelle tension (un pas éteint garde la sienne).
inline constexpr bool kCorruptChangesOffBitVoltages = false;

// --- CV des séquenceurs (§8.2, §8.5) ---
inline constexpr float kCvRangeMinScale = 0.1f; // CV RANGE au minimum : ±0,5 V (environ 1 octave)
inline constexpr int kDefaultQuantizerMode = 2; // index 0..15 : mode n°3, majeur
inline constexpr int kQuantizerModeAfterClear = kDefaultQuantizerMode; // « efface la séquence » (§8.4)
inline constexpr int kMidiRootNote = 60; // Note On : décalage de fondamentale = note − 60

// --- Horloge (§8.6) ---
inline constexpr float kTempoMinStepsPerMinute = 10.0f;
inline constexpr float kTempoMaxStepsPerMinute = 600.0f;
inline constexpr float kTempoDefaultStepsPerMinute = 120.0f;
// Hypothèse : division N = un pas dure N doubles croches (division 1 = 1/16, soit 6 impulsions à 24 PPQN).
inline constexpr double kHostQuartersPerStepAtDivision1 = 0.25;

// --- Bip de debug de la phase 2 (remplacé par la voix en phase 3) ---
inline constexpr float kBeepFrequencyAtZeroVolt = 261.63f; // do 4, puis 1 V/octave
inline constexpr float kBeepDecaySeconds = 0.12f;
inline constexpr float kBeepLevel = 0.3f;
inline constexpr float kBeepPan = 0.7f; // part de SEQ1 à gauche et de SEQ2 à droite en stéréo
} // namespace dedale::calibration
