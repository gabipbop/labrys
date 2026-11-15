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

// --- Hauteurs et FM (§3) ---
inline constexpr float kEgMaxVolts = 8.0f;    // crête des enveloppes
inline constexpr float kQtzZoneStart = 0.97f; // SEQ AMT : derniers 3 % = gain verrouillé à 1 (cran QTZ)
inline constexpr float kEgPitchOctavesPerVolt = 0.5f; // EG1 AMT à fond : 4 octaves de balayage
inline constexpr float kTzfmMaxIndex = 4.0f;          // profondeur de TZFM avec FM AMT à fond

// --- Bruit (§3.4) ---
inline constexpr float kNoiseMinHz = 100.0f;
inline constexpr float kNoiseMaxHz = 12000.0f;
inline constexpr float kNoiseQ = 0.7f;
inline constexpr float kNoiseRmsVolts = 1.7f;

// --- Mixeur saturant (§3.5), niveaux normalisés (1 = 5 V) ---
inline constexpr float kMixerMaxGain = 5.0f;        // gain d'un canal en butée (1 à midi)
inline constexpr float kMixerChannelCeiling = 1.5f; // plafond de saturation d'un canal
inline constexpr float kMixerSumCeiling = 2.0f;     // plafond de la somme
inline constexpr float kMixerAsymmetry = 0.1f;      // offset avant saturation : harmoniques paires

// --- Wavefolder (§4) ---
enum class FolderModel
{
    softTwoStage, // deux plis triangulaires à coins adoucis
    sine          // pli sinus simple, pour comparaison
};
inline constexpr FolderModel kFolderModel = FolderModel::softTwoStage;
inline constexpr float kVcwInputGain = 0.7f;        // gain d'entrée à FOLD = 0 (sous le premier pli)
inline constexpr float kVcwFoldGain = 9.0f;         // gain ajouté avec FOLD à fond
inline constexpr float kVcwStage2Gain = 1.5f;       // gain ajouté avant le second étage avec FOLD à fond
inline constexpr float kVcwBiasRange = 1.0f;        // offset avec BIAS en butée
inline constexpr float kVcwCornerSharpness = 0.97f; // 1 = coins vifs, plus bas = coins ronds
inline constexpr float kVcwAsymmetry = 0.05f;       // offset entre les étages, proportionnel à FOLD
inline constexpr bool kVcwZeroColour = true;        // coloration résiduelle à FOLD = 0
inline constexpr float kVcwZeroColourDrive = 0.4f;
inline constexpr float kVcwOutputDrive = 0.8f; // saturation finale

// --- Filtre (§5) ---
inline constexpr float kVcfMinDamping = 0.12f;        // 1/Q à résonance max (Q ≈ 8, pas d'auto-oscillation)
inline constexpr float kVcfStateCeilingVolts = 12.0f; // niveau où les intégrateurs saturent
inline constexpr float kCutoffOctavesPerVolt = 1.0f;

// --- VCA, enveloppes, sortie (§6, §7) ---
inline constexpr float kVcaExponent = 1.5f;        // 1 = linéaire
inline constexpr float kEgAttackSeconds = 0.0007f; // micro-attaque anti-clic
inline constexpr float kEgSmoothingHz = 2000.0f;   // lissage de la CV de sortie
inline constexpr float kOutputHeadroom = 0.7f;     // gain avant la sortie : le patch init reste sous le genou
inline constexpr float kOutputClipKneeVolts = 3.5f; // l'écrêtage de sécurité commence ici, plafond à 5 V
inline constexpr int kOversamplingStages = 2;       // ×4 (réglable en phase 6)
inline constexpr float kParamSmoothingSeconds = 0.02f;

// --- Patch bay (§9) ---
inline constexpr float kTriggerPulseSeconds = 0.001f; // largeur des impulsions SEQ TRIG et CLOCK (0 → +5 V)
inline constexpr float kRailVolts = 10.0f;            // U MIX sature aux rails, comme le circuit
// Hypothèse : un câble dans EG2 TRIG isole EG2, qui ne suit plus ni TRIGGER ni les triggers des séquenceurs.
inline constexpr bool kEg2TrigJackIsolatesEg2 = true;
} // namespace dedale::calibration
