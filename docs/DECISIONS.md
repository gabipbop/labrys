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

## ADR-011 — Cœur pur `dedale_core` et outil `DedaleRender` (2026-11-15)
- **Décision** : le séquenceur, le quantizer et les horloges vivent dans une bibliothèque statique `dedale_core`
  sans aucun include JUCE, liée par le plugin, les tests et les outils. `DedaleRender` lie la cible statique
  `Dedale` (code partagé du plugin) et reprend ses includes et définitions, sans recompiler les modules JUCE.
- **Conséquences** : les tests du séquenceur compilent en une seconde et ne dépendent pas de JUCE. Rien n'est
  extrait dans ARIANE pour l'instant (pas de second instrument pour juger de ce qui est générique).

## ADR-012 — Hypothèses du séquenceur (2026-11-15)
Vérifié dans le manuel : CORRUPT ne change la tension que des pas **actifs** (« bits that are on ») ; le bit flip
touche n'importe quel pas ; en chain, les deux lanes parcourent les mêmes 16 pas avec chacune son play head,
son horloge et son CV RANGE. Le reste est interprété :
- CORRUPT agit **avant** la lecture du pas (`kCorruptTiming`). Un tirage de flip réussi remplace le tirage de tension.
- Après RESET (et au démarrage), le **prochain tick joue le pas 1** au lieu de sauter au pas 2.
- ADVANCE joue le pas atteint (trigger si actif) mais n'applique pas CORRUPT.
- Annulation des rotations (`BIT SHIFT + RESET`) : un compteur par lane ; il est remis à zéro quand la longueur
  ou le mode chain change (la position courante devient l'origine).
- En chain, l'offset du write head boucle sur la longueur totale (jusqu'à 15).
- « Effacer la séquence » remet le quantizer sur le mode par défaut (majeur), pas sur « non quantifié ».
- **Écart à la spec §8.2** : la polarité unipolaire est appliquée **avant** le quantizer. Après, `v × 0,5` diviserait
  les intervalles par deux et sortirait de la gamme.
- Les combinaisons de boutons (§8.4) ne sont pas dans le séquenceur : il expose une méthode par action, l'UI
  (phase 5) fera la traduction.

## ADR-013 — Horloge et périmètre de la phase 2 (2026-11-15)
- **Division hôte** : division N = un pas dure N doubles croches (1 = 1/16, 4 = noire, 16 = ronde). C'est la lecture
  la plus simple de « 1 à 16, défaut 1/16 » avec 24 PPQN ; à confirmer à l'usage.
- `SYNC = HOST` avec le transport arrêté : l'horloge interne prend le relais (on peut jouer DAW à l'arrêt).
  Le démarrage du transport ne remet pas le séquenceur au pas 1 (comme RUN/STOP sur le hardware).
- **Reporté** : MIDI clock, Start/Stop/Continue, canal MIDI et MIDI out (phase 6) ; jacks CLOCK/RESET/BIT FLIP
  (phase 4) ; sauvegarde de la séquence dans le state (phase 6, seuls les paramètres sont sauvés) ; golden tests
  (phase 3, quand il y aura un vrai son à figer).
- **Provisoire** : sans boutons avant la phase 5, le plugin tire un motif aléatoire au chargement.

## ADR-014 — Python pour `tools/analyse.py` (2026-11-15)
- **Constat** : le Python système n'a ni numpy ni scipy. **Décision** : environnement local `tools/.venv`
  (gitignoré), créé par `python3 -m venv tools/.venv && tools/.venv/bin/pip install numpy scipy matplotlib`.
- La mesure d'aliasing (§15.6) arrivera avec les oscillateurs (phase 3).
