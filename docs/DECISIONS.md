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
- **À vérifier en phase 1** : si les CLT suffisent pour builder l'AU et faire tourner `auval`.
