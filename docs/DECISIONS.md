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

## ADR-015 — Voix : structure et hypothèses (2026-11-15)
- **Structure** : un module par étage dans `Source/DSP` (sans JUCE, dans `dedale_core`), assemblés par `Voice` avec
  les normalisations par défaut en dur. Le processor fait tourner séquenceur et enveloppes à la fréquence de base,
  appelle `Voice::setControls()` une fois par échantillon de base, puis `Voice::processSample()` ×4 dans un bloc
  `juce::dsp::Oversampling` (demi-bandes IIR polyphase, 2 étages). Les contrôles sont tenus pendant les 4
  sous-échantillons. La latence du suréchantillonneur (quelques échantillons) n'est pas déclarée à l'hôte.
- **Écart à la spec §6** : DC-block **avant** l'écrêtage de sécurité (et non après), pour garantir une sortie dans
  ±1. L'écrêtage est transparent jusqu'à `kOutputClipKneeVolts` ; `kOutputHeadroom` garde le patch init dessous.
- **EG TRIG MIX** : chaque flux est à 100 % jusqu'à midi puis descend vers 0 (les deux à 100 % à midi), au lieu d'un
  fondu linéaire qui donnerait 50 % chacun à midi.
- **Filtre** : résonance à 0 = Butterworth (−3 dB au cutoff) ; à fond, Q ≈ 8 sans auto-oscillation. Le passe-bande
  est compensé en `√` de l'amortissement : gain unité sans résonance, pic modéré avec.
- **Wavefolder** : deux plis triangulaires à coins arrondis (`asin(c·sin)`), second étage plus poussé et légèrement
  décalé quand FOLD monte. Variante `sine` gardée derrière `kFolderModel`. Le taux d'harmoniques n'est pas monotone
  avec FOLD (les plis annulent la fondamentale à certains réglages) : c'est le comportement normal d'un folder.
- **Modulations** : EG1 vers les hauteurs = 0,5 octave par volt à fond (4 octaves) ; EG1 et SEQ2 vers le cutoff =
  1 V/octave ; EG1 et SEQ1 vers FOLD ramenés à la course du knob.
- **Golden tests en C++** (`DedaleGoldenTests`) plutôt qu'avec `analyse.py` : la CI n'a pas besoin de Python.
  Ils rendent les 4 premières secondes des 14 scénarios et comparent crête, RMS, DC et passages à zéro.
- **Aliasing mesuré** : −32,7 dB d'énergie non harmonique sur `alias_fold_high` (2,5 kHz, FOLD à fond, ×4), le cas
  le plus dur. À réévaluer avec le mode ×8 (phase 6) si c'est audible.
- **Reporté** : drift analogique, interrupteur de coloration du VCW, qualité d'oversampling réglable (phase 6) ;
  U MIX et `umix1_lvl`, SYNC du MOD VCO, entrées audio directes (phase 4, ils n'ont de sens qu'avec les jacks) ;
  bouton TRIGGER (phase 5 ; d'ici là, un Note On à l'arrêt déclenche les deux enveloppes).
- Phase démarrée à la demande de Gaby avant ses réponses au checkpoint de la phase 2.

## ADR-016 — Patch bay et moteur (2026-11-15)
- **Structure** : `PatchBay` est un tableau de tensions de sortie (12) et un tableau de sources par entrée (20).
  Lire une entrée rend la tension de la sortie reliée, ou la valeur de normalisation passée par le module. Pas de
  graphe ni de tri : chaque module lit la dernière valeur écrite, donc une sortie pas encore recalculée dans
  l'échantillon courant est lue avec **un échantillon de retard**. C'est ce qui rend les boucles de feedback sûres.
- **Deux cadences** : les entrées trigger/gate (CLOCK, BIT FLIP, RESET, TRIGGER, EG2 TRIG) et les sorties du
  séquenceur et des enveloppes sont traitées à la fréquence de base dans `Engine::processControl`. Les entrées audio
  et CV de la voix sont relues à chaque échantillon suréchantillonné dans `Voice::processSample`. Une entrée CV sans
  câble garde sa valeur calculée à la fréquence de base (aucun coût), une entrée patchée est recalculée par
  échantillon (`exp2`, `tan` du filtre) : c'est ce qui permet la FM et la modulation de cutoff à l'audio-rate.
- **`Engine`** (pur, dans `dedale_core`) reprend l'orchestration qui vivait dans le processor. Le processor ne garde
  que JUCE : paramètres, horloge interne/hôte, MIDI, suréchantillonnage. Les golden de la phase 3 passent sans
  changement après ce déplacement.
- **Hypothèses** :
  - un câble dans **EG2 TRIG** isole EG2, qui ne suit plus ni TRIGGER ni les triggers des séquenceurs
    (`kEg2TrigJackIsolatesEg2`). Le manuel dit seulement que TRIGGER ne déclenche plus que EG1 ;
  - CV externes dans FOLD et BLEND : ±5 V couvrent toute la course du knob (EG1 : 8 V pour toute la course) ;
  - SEQ TRIG et CLOCK sortent des impulsions de 1 ms à +5 V ; CLOCK suit l'horloge maître même à l'arrêt ;
  - U MIX sature à ±10 V (`kRailVolts`), sinon une sortie rebouclée sur U MIX 2 à gain unité divergerait ;
  - les jacks 1V/OCT n'ont pas d'atténuateur (comme sur le panneau) : on passe par U MIX 1 LVL pour doser une FM.
- **Jacks exposés à la DAW** : rien en phase 4, on suit la spec §9 (extension facultative de la phase 6).
- **Reporté** : câbles dans le state et changement de câble pendant la lecture (FIFO UI → audio) : phases 5 et 6.
  Réglage « RESET rappelle le buffer » : phase 6.

## ADR-017 — Interface (2026-11-15)
- **Direction** : après deux séries de maquettes, Gaby a choisi un rendu « synthé matériel classique » (esprit
  Mariana) : façade noire, modules encadrés avec cartouche, knobs à jupe noire et capuchon métal, libellés
  au-dessus des réglages, boutons lumineux. **Un seul thème, le noir** (écart au §11.2, qui en prévoyait deux).
  Rien de la marque Moog : ni logo, ni nom, ni police, ni bois.
- **Habillage** : la première version (Barlow Condensed, LEDs rondes, cuivre et ambre) a été jugée « trop gentille
  et trop IA ». Parmi trois habillages plus industriels rendus par le plugin, Gaby a retenu : police **Chakra
  Petch** (OFL, écart au §11.2 qui citait Barlow Condensed ou Space Grotesk), **LEDs et boutons à pans coupés**,
  **accent rouge** (au lieu du cuivre/orange du §11.2), cadres de modules arrondis, **flancs noirs vissés**, vis
  apparentes aux coins des modules. Le write head est repéré par deux traits cyan au-dessus et au-dessous de la LED.
- **Structure** : le panneau est dessiné dans un repère fixe 960 × 444 (`UI/Layout.h` donne toutes les positions) et
  l'éditeur lui applique une échelle (75 % à 200 %, proportions fixes, taille mémorisée dans le state). Tout est
  vectoriel. La façade statique est mise en cache dans une image ; LEDs, afficheur, oscilloscope et câbles sont
  des composants à part, redessinés seulement quand leur état change (minuterie à 30 Hz).
- **Interface → audio** : les boutons passent par `ButtonComboHandler` (thread de l'interface), qui produit des
  actions. Les réglages (quantizer, division) modifient les paramètres ; le reste part dans une FIFO sans verrou
  lue en début de bloc. Les **câbles** sont une table d'atomics tenue par le processor, recopiée dans la patch bay
  à chaque bloc : elle fonctionne même si l'audio est arrêté et sera la donnée à sauvegarder en phase 6.
- **Audio → interface** : état du séquenceur publié en atomics à chaque bloc, oscilloscope par FIFO.
- **Combinaisons à la souris** : on ne peut pas tenir deux boutons. Le bouton SHIFT verrouille BIT SHIFT, LENGTH
  et BUFFER (ils restent tenus jusqu'au clic suivant), les autres boutons restent des appuis brefs. ADVANCE reste
  tenu 0,6 s après son clic, le temps de cliquer RESET pour la combinaison à trois boutons. Sans SHIFT, un
  bouton se comporte comme sur le matériel : tenu tant que la souris est enfoncée (BIT FLIP, BUFFER 1 s).
- **Câbles** : glisser d'une prise à une autre ; glisser depuis une entrée câblée déplace le câble ; clic droit ou
  double-clic retire. Couleur par type de signal de la sortie (audio, CV, trigger).
- **Vérification** : `DedaleRender --ui <dossier>` capture le panneau en PNG sans DAW. pluginval niveau 10 avec
  ses tests d'éditeur passe en local ; la CI les saute (pas d'écran). Les interactions souris ne sont pas
  testées automatiquement : à essayer dans la DAW.
- **Reporté** : rappel du buffer qui ne met pas à jour les paramètres `chain` et `quant_mode` affichés par la DAW
  (phase 6, avec le state complet) ; clignotement du write head en rythme.

## ADR-018 — Sauvegarde du projet, réglages globaux, presets, MIDI, budget de CPU (2026-11-15)
- **État sauvegardé** : l'arbre des paramètres, plus trois enfants : `SEQUENCER` (bits, tensions, longueurs,
  têtes, rotations, buffer, quantizer, chain), `CABLES` (table des 20 entrées) et `SETTINGS` (réglages globaux),
  avec `stateVersion = 1`. Lecture tolérante : un enfant absent garde les valeurs courantes (projets d'avant la
  phase 6), une valeur hors bornes est ramenée dans les bornes (`GenerativeSequencer::setState`, `unpack`).
  La seed du générateur aléatoire n'est pas sauvegardée : la séquence est retrouvée à l'identique, ses mutations
  futures ne sont pas rejouées.
- **Échange avec le thread audio** : l'interface lit et écrit l'état du séquenceur sous un `SpinLock` que le thread
  audio ne prend qu'en `tryLock` (il n'attend jamais). Une restauration est appliquée au début du bloc suivant ;
  d'ici là, `sequencerState()` rend l'état en attente, pour qu'un « charger puis sauver » immédiat soit fidèle.
- **Paramètres pilotés par le moteur** : `chain`, `quant_mode` (rappel du buffer) et `run` (Start/Stop MIDI) peuvent
  changer côté audio. Le thread audio n'applique un paramètre que lorsqu'il change, et une minuterie du processor
  (15 Hz, thread de l'interface) recale les paramètres sur l'état réel.
- **Réglages globaux** : rangés dans un entier atomique (`Settings.h`). Changer le suréchantillonnage suspend le
  traitement le temps de reconstruire le suréchantillonneur. **Drift activé par défaut** (±2 cents, `kDriftCents`).
- **Presets** : un preset est un patch (réglages et câbles). Il ne touche ni à la séquence, ni au transport (RUN,
  TEMPO, synchro, division), ni aux réglages globaux. Presets d'usine dans le code (`Presets.cpp`), presets
  utilisateur en XML dans `~/Library/Application Support/Raw Powa Studio/Dedale/Presets`.
- **MIDI** : priorité des horloges MIDI > hôte > interne. Start et Continue arment l'horloge MIDI tout de suite,
  sinon l'horloge interne jouait un pas de trop avant la première impulsion (bug trouvé par les tests).
  Sortie MIDI : note = do 4 + CV quantifiée, durée fixe (`kMidiOutGateSeconds`), SEQ2 sur le canal suivant.
- **Budget de CPU** : `DedaleBench` mesure le meilleur de trois passes et échoue au-delà de 5 % d'un cœur. Mesuré :
  1,65 % (patch init) et 2,97 % (patch lourd, quatre modulations à l'audio-rate) en ×4 à 48 kHz. Le test n'est
  enregistré que sur un build Release, avec l'étiquette `bench`. La CI l'exclut : sur le runner partagé, le même
  code a mesuré 4,56 % et 8,19 %. Elle lance `DedaleBench` à titre indicatif (budget 100 %, pour voir les chiffres).
- **Non fait** : jacks exposés à la DAW (macros, sidechain), option de la spec à décider avec Gaby ; resserrage du
  module Wavefolder (question posée à Gaby dans la checklist de la phase 5).
