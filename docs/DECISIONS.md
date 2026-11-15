# Journal des décisions

Format court : contexte, décision, conséquences. Une entrée par choix, la plus récente en bas.

## ADR-001 — Monorepo LABRYS avec ARIANE dès la phase 0 (2026-11-15)
- **Contexte** : DÉDALE est le premier de cinq instruments (§16bis).
- **Décision** : `/libs/ariane` (partagé), `/plugins/<nom>`, `/tutorial`, `/docs`. Les noms, codes plugin et bundle IDs
  sont centralisés dans `libs/ariane/cmake/SuiteIdentity.cmake`, qui vérifie au configure que chaque code fait
  4 caractères ASCII et qu'il n'y a pas de doublon.
- **Conséquences** : renommer un instrument = modifier un seul fichier.

## ADR-002 — ARIANE est une cible INTERFACE tant qu'elle est vide (2026-11-15)
- **Contexte** : la spec demande une cible statique, mais aucun module n'est encore extrait.
- **Décision** : `ariane` est `INTERFACE` en phase 0 ; elle passera `STATIC` avec son premier fichier source.
- **Conséquences** : les cibles lient déjà `labrys::ariane`, rien à changer chez elles au passage en STATIC.

## ADR-003 — Spec gardée sous le nom `docs/SPEC_DEDALE.md` (2026-11-15)
- **Contexte** : §15.3 demande une copie dans `docs/SPEC.md`, §0bis nomme les futures specs `docs/SPEC_<NOM>.md`.
- **Décision** : un seul fichier, `docs/SPEC_DEDALE.md`, pas de copie (deux copies finiraient par diverger).
- **Conséquences** : `CLAUDE.md` pointe vers `docs/SPEC_DEDALE.md`.

## ADR-004 — Presets CMake (2026-11-15)
- **Décision** :
  - `debug` : boucle de dev ;
  - `tests` : Debug avec warnings = erreurs, à lancer avant chaque commit ;
  - `asan` : Debug + AddressSanitizer + UBSan ;
  - `release` : universal arm64 + x86_64, warnings = erreurs, utilisé par la CI.
  Tous construisent les tests ; `ctest` échoue s'il ne trouve aucun test. Les dépendances FetchContent sont
  partagées entre presets dans `build/_deps` (et mises en cache en CI).
- **Conséquences** : un test « smoke » existe en phase 0 pour prouver la chaîne CMake + Catch2 + ctest ;
  il sera supprimé avec les premiers vrais tests (phase 2).

## ADR-005 — Catch2 v3.8.1 (2026-11-15)
- **Décision** : Catch2 v3 (plutôt que GoogleTest), tag figé v3.8.1, découverte des tests avec `catch_discover_tests`.

## ADR-006 — Outillage macOS (2026-11-15)
- **Constat** : seules les Command Line Tools sont installées (pas Xcode complet). `clang-format` et `poppler`
  ont été installés via Homebrew. `pnpm` manque (nécessaire seulement en phase 8).
- **Vérifié en phase 1** : les CLT suffisent pour builder VST3/AU/Standalone et faire tourner `auval`.

## ADR-007 — Nom vu par la DAW en ASCII (2026-11-15)
- **Contexte** : avec `PLUGIN_NAME "DÉDALE"`, le VST3 ne se charge pas dans les hôtes JUCE (pluginval :
  « Unable to load VST-3 plug-in file ») et le nom s'affiche mal (« DￃﾉDALE »). Le même build avec « DEDALE » passe.
- **Décision** : `SuiteIdentity.cmake` distingue `<ID>_HOST_NAME` (ASCII, vérifié au configure : DEDALE, PHEDRE...)
  et `<ID>_DISPLAY_NAME` (accentué, dans l'interface du plugin via `LABRYS_DISPLAY_NAME`).
- **Conséquences** : la DAW liste « Raw Powa Studio: DEDALE » ; le panneau affiche « DÉDALE ».

## ADR-008 — JUCE 8.0.15 (2026-11-15)
- **Contexte** : JUCE 9 existe (9.0.5), la spec demande JUCE 8.
- **Décision** : dernière JUCE 8, tag figé 8.0.15. Passage à JUCE 9 = décision à part (licence et API à relire).
- Les modules JUCE sont compilés dans la cible plugin : nos warnings stricts (`ariane_warnings`) sont appliqués
  **par fichier** à nos sources seulement, pour ne pas rendre la build dépendante des warnings de JUCE.

## ADR-009 — Validation des plugins (2026-11-15)
- `tools/pluginval.sh <plugin> [niveau]` télécharge pluginval v1.0.4 dans `tools/` (gitignoré) et lance la validation
  (tests GUI ignorés, pour pouvoir tourner en CI). Le preset `debug` copie les plugins dans `~/Library/Audio/Plug-Ins`
  (nécessaire pour `auval` et la DAW).
- La CI lance pluginval niveau 5 (VST3 + AU) et `auval`. Niveau 10 en local avant chaque merge.
- Coût CI : runner standard sur repo public (gratuit), `concurrency` qui annule les runs dépassés, pas de CI pour
  les changements de docs seuls, timeout 45 min.

## ADR-010 — Les plugins sont rangés sous « LABRYS » dans les DAW (2026-11-15)
- **Contexte** : les hôtes rangent les plugins par fabricant (`COMPANY_NAME` JUCE : vendor VST3, préfixe du nom AU).
  Avec « Raw Powa Studio » (§15.1), DÉDALE apparaissait dans un dossier Raw Powa Studio ; Gaby préfère un dossier LABRYS.
- **Décision** : fabricant vu par la DAW = `LABRYS_VENDOR_NAME` (= « LABRYS ») ; Raw Powa Studio reste l'éditeur
  (copyright, bundle IDs `com.rawpowastudio.*`, code fabricant `Rwps`).
- **Conséquences** : les DAW affichent « LABRYS: DEDALE ». Les identifiants (codes AU, UID VST3) ne changent pas,
  donc un projet existant retrouve le plugin. TORO, hors suite, reste sous Raw Powa Studio.
