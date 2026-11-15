# LABRYS — instructions pour Claude Code

Suite de plugins (VST3/AU/Standalone, JUCE 8, C++20) de **Raw Powa Studio**. Premier instrument : **DÉDALE**.
Spec complète : [docs/SPEC_DEDALE.md](docs/SPEC_DEDALE.md) (source de vérité du projet). État courant : [docs/PROGRESS.md](docs/PROGRESS.md).

## DÉDALE en bref (§0 de la spec)
Synthé **monophonique** sans clavier, piloté par **deux séquenceurs génératifs 8 pas à registre à décalage**.
Sources : VCO sinus + MOD VCO triangle (TZFM) + ring mod + bruit → **mixeur saturant**.
Deux chemins : **wavefolder (VCW)** et **SVF 2 pôles LP↔BP (VCF)**, chacun avec son VCA, en parallèle ou en série (ORDER), mélangés par **BLEND**.
Deux enveloppes **decay-only**. **Patch bay virtuelle** de 32 points (20 entrées, 12 sorties) avec normalisations.
Caractère visé : « West Coast », agressif, métallique, percussif.

## Règles non négociables
- Identité visuelle **originale** : aucun logo, nom, typo, couleur, flanc en bois ni sérigraphie Moog/Labyrinth (UI, code, métadonnées, presets, tutoriel).
- Les manuels de `docs/reference/` (gitignoré) servent à comprendre, **jamais à recopier** (texte, schémas, illustrations).
- En cas de doute sur le comportement d'origine : vérifier dans le manuel (`grep` sur les `.txt`), noter la conclusion dans `docs/DECISIONS.md`.
- **TORO** est un autre repo : n'y touche pas, ne réutilise pas ses codes plugin.

## Arborescence (monorepo)
```
/libs/ariane            bibliothèque partagée (DSP, séquençage, patch bay, UI, outillage)
  /cmake/SuiteIdentity.cmake   SEUL endroit où sont définis noms, codes plugin, bundle IDs
                               (nom DAW en ASCII, nom accentué seulement dans l'UI : ADR-007)
  /cmake/ArianeWarnings.cmake  cible ariane_warnings (-Wall -Wextra -Wpedantic ...)
/plugins/dedale         code propre à DÉDALE (Source/{DSP,Sequencer,Patch,UI}, Tests/)
/tutorial               site web interactif (phase 8)
/tools                  scripts (render, analyse, pluginval)
/docs                   spec, ARCHITECTURE, DECISIONS, CALIBRATION, PROGRESS, LISTENING_CHECKLIST
```
N'extrais dans ARIANE que ce qui est clairement générique, et note chaque extraction dans `DECISIONS.md`.

## Commandes
```bash
cmake --preset debug && cmake --build --preset debug && ctest --preset debug   # boucle normale
cmake --preset tests && cmake --build --preset tests && ctest --preset tests   # debug strict (-Werror), avant commit
cmake --preset asan && cmake --build --preset asan && ctest --preset asan      # ASan + UBSan
cmake --preset release && cmake --build --preset release && ctest --preset release  # universal, -Werror (= CI)
git ls-files '*.cpp' '*.h' | xargs clang-format -i                              # formatage
tools/pluginval.sh build/release/plugins/dedale/Dedale_artefacts/Release/VST3/Dedale.vst3 10  # pluginval
auval -v aumu Ddle Rwps                                                         # AU (après build debug, qui installe)
build/debug/plugins/dedale/DedaleRender renders 7                                # rendus WAV (dossier, seed)
tools/.venv/bin/python -I tools/analyse.py renders/*.wav --png renders/spectrograms             # mesures
build/debug/plugins/dedale/DedaleRender --golden plugins/dedale/Tests/golden/metrics.json        # refige les golden
```
Dépendances via `FetchContent` à **tag figé** (jamais `main`), partagées dans `build/_deps`.
Versions figées : JUCE 8.0.15, Catch2 v3.8.1, pluginval v1.0.4.

## Conventions de code (§12)
- Code DSP isolé de JUCE (`Source/DSP/` sans include JUCE, sauf `juce::dsp::Oversampling` si besoin).
- Chaque module DSP : `prepare(sampleRate, maxBlock)`, `reset()`, `processSample(...)`, testé isolément.
- Tensions de modulation internes en **volts** (`1.0f` = 1 V). Audio ±5 V en interne, ramené à ±1.0 en sortie.
- **Audio thread** : zéro allocation, zéro lock, zéro log. UI↔audio par `std::atomic` et `juce::AbstractFifo`.
- `juce::ScopedNoDenormals` dans `processBlock`.
- Oversampling : sources + VCW + VCF dans un seul bloc suréchantillonné ; séquenceur et enveloppes à la fréquence de base.
- IDs de paramètres APVTS en `snake_case`, **jamais renommés** après la v1.
- Logique pure (séquenceur, quantizer, horloges) dans la bibliothèque `dedale_core`, sans aucun include JUCE.
- Constantes à calibrer à l'oreille : toutes dans `Calibration.h`, documentées dans `docs/CALIBRATION.md`.
- Zéro warning. Style : `.clang-format` (Allman, 4 espaces, 110 colonnes).

## Coût GitHub
Gaby ne doit rien payer : repo public, runners standard seulement, pas de CI pour les docs seules, `concurrency`.
Si un job est refusé pour facturation, prévenir Gaby au lieu de relancer.

## Git (§15.4)
- `main` toujours vert, **jamais de commit direct**. Une branche par phase (`phase-2-sequencer`...).
- Conventional Commits **en français** : `feat(seq): ...`, `fix(vcf): ...`, `test(quant): ...`, `docs: ...`.
- Fin de phase : PR (`gh pr create`) avec ce qui est fait, tests, points à écouter, hypothèses ouvertes ;
  CI verte ; **attendre la validation de Gaby** avant le merge (squash). Tag `v0.N.0` à chaque phase mergée.

## Boucle de travail par tâche (§15.5)
1. Relire la section de spec + `PROGRESS.md`.
2. Écrire dans `PROGRESS.md` la tâche, les fichiers touchés, les tests prévus (5 à 10 lignes).
3. **Tests d'abord** pour toute logique pure (séquenceur, quantizer, horloge, routeur, sérialisation).
4. Implémenter le minimum, puis refactorer.
5. Build debug + release, zéro warning.
6. `ctest` complet + preset `asan`.
7. Vérification audio objective (§15.6) pour tout ce qui touche au son.
8. `pluginval` niveau 5 (10 avant merge) et `auval -v aumu Ddle Rwps` (dès la phase 1).
9. Commit, puis mise à jour de `PROGRESS.md`, `CHANGELOG.md`, `DECISIONS.md` si un choix a été fait.

Si une étape échoue deux fois pour la même raison : s'arrêter, résumer dans `PROGRESS.md`, demander à Gaby.

## Vérifier le son sans l'écouter (§15.6)
Cible `DedaleRender` (WAV 48 kHz/24 bits dans `renders/`), `tools/analyse.py` (environnement `tools/.venv`, ADR-014),
golden tests `DedaleGoldenTests` en métriques JSON (changement volontaire = commit séparé et justifié), spectrogrammes PNG aux checkpoints.

## Checkpoints avec Gaby (§15.7)
Fin des phases 2, 3, 4, 5, 7 : PR + 3 à 6 rendus WAV nommés + 5 à 10 questions dans `LISTENING_CHECKLIST.md`.
Ne pas passer à la phase suivante sans réponse. Noter les retours dans `CALIBRATION.md`.

## Hypothèses (§14)
Tout point incertain : version la plus plausible, isolée derrière une constante ou un enum dans `Calibration.h`,
signalée dans le rapport de fin de phase.
