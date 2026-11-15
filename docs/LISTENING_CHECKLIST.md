# Points d'écoute

À chaque checkpoint (fin des phases 2, 3, 4, 5, 7) : 5 à 10 questions précises, avec les rendus WAV à comparer.

## Checkpoint phase 2 — Séquenceur

Rendus : `cmake --build --preset debug && build/debug/plugins/dedale/DedaleRender renders 7` (WAV dans `renders/`).
Le son est un **bip sinus de debug** : on écoute seulement le rythme et les notes, pas le timbre. SEQ1 est plutôt
à gauche, SEQ2 plutôt à droite. Tous les rendus tournent à 4 pas par seconde, motif de départ SEQ1 `.xx.xx.x`.
Spectrogrammes : `docs/checkpoints/phase-2/`.

| Fichier | Contenu |
|---|---|
| `seq_corrupt_0.wav` | SEQ1 seul, CORRUPT à 0, sync hôte simulée (120 BPM, croches) |
| `seq_corrupt_40.wav` | idem, CORRUPT à 40 % |
| `seq_corrupt_75.wav` | idem, CORRUPT à 75 % |
| `seq_polymeter_8_5.wav` | SEQ1 sur 8 pas contre SEQ2 sur 5 pas |
| `seq_chain_16.wav` | CHAIN : les deux lanes jouent la même séquence de 16 pas |
| `seq_quant_modes.wav` | SEQ1, CV RANGE à 50 %, le quantizer change toutes les 6 s : non quantifié, chromatique, majeur, pentatonique, quads |

Questions :
1. `seq_corrupt_0.wav` : la boucle de 8 pas se répète-t-elle à l'identique, sans note qui bouge ?
2. `seq_corrupt_40.wav` : les notes dérivent-elles à un rythme musical (ni trop figé, ni trop agité) pendant que le
   rythme reste intact ?
3. `seq_corrupt_75.wav` : la mutation du rythme est-elle convaincante, ou trop chaotique pour un réglage aux trois
   quarts ? (Sur le hardware, à fond, un pas a une chance sur deux de changer.)
4. `seq_corrupt_40.wav` : quand une note change, l'entends-tu tout de suite, sur le pas en cours ? C'est l'hypothèse
   « CORRUPT avant la lecture » ; l'alternative la ferait entendre un tour plus tard.
5. `seq_polymeter_8_5.wav` : le décalage 8 contre 5 est-il clair, avec un cycle complet toutes les 10 secondes ?
6. `seq_chain_16.wav` : entends-tu bien une phrase de 16 pas (4 s) qui enchaîne le motif de SEQ1 puis celui de SEQ2 ?
7. `seq_quant_modes.wav` : chaque gamme est-elle reconnaissable ? Le passage en majeur puis pentatonique sonne-t-il
   juste ? L'étendue des notes à CV RANGE 50 % te paraît-elle bonne (environ 5 octaves de course totale) ?
8. Dans Ableton (paramètres du plugin : `run` sur on, transport en lecture) : la division 1 doit donner des doubles
   croches calées sur le métronome, 2 des croches, 4 des noires. Est-ce la lecture que tu attends de « DIVISION » ?
9. Transport arrêté avec `run` sur on, le séquenceur continue sur son horloge interne (`tempo`). Préfères-tu qu'il
   s'arrête avec le transport ? Et faut-il repartir du pas 1 à chaque lecture ?
10. Si tu as le hardware ou des enregistrements : les gammes 7 (diminished 6th), 9 (hirajoshi) et 15 (handpan)
    correspondent-elles ? Ce sont des interprétations.

## Checkpoint phase 3 — Voix

Mêmes commandes de rendu. Les rendus `seq_*` de la phase 2 passent maintenant par la vraie voix (patch init) : les
questions de la phase 2 restent valables, avec un son plus parlant que le bip. Spectrogrammes :
`docs/checkpoints/phase-3/`. Tous les rendus : motif SEQ1 `.xx.xx.x`, 4 pas par seconde.

| Fichier | Contenu |
|---|---|
| `init_patch.wav` | Réglages par défaut, sans rien toucher |
| `tzfm_sweep.wav` | Note fixe, chemin VCW sans pli, FM AMT de 0 à fond (MOD VCO à l'unisson du VCO) |
| `fold_sweep.wav` | Note fixe, chemin VCW seul, FOLD de 0 à fond |
| `fold_bias_sweep.wav` | FOLD à 0,6, BIAS de −1 à +1 |
| `vcf_res_sweep.wav` | Chemin VCF seul, VCO + bruit, l'enveloppe ouvre le filtre, RESONANCE de 0 à fond |
| `mixer_drive.wav` | Chemin VCF seul, VCO LVL de midi à fond |
| `order_compare.wav` | 4 s en PARALLEL, 4 s en VCW>VCF, 4 s en VCF>VCW |
| `alias_fold_high.wav` | Note tenue à 2,5 kHz, FOLD à fond : test d'aliasing |

Questions :
1. `init_patch.wav` : est-ce un bon son de départ, percussif et métallique, ou trop sage / trop agressif ? Le niveau
   de sortie te convient-il ?
2. `init_patch.wav` : l'attaque des notes est-elle franche sans clic ? Entends-tu un clic quand deux notes
   s'enchaînent vite ?
3. `fold_sweep.wav` : le début (FOLD à 0) est-il assez propre ? La montée est-elle progressive ? Vers la fin, est-ce
   trop dur, ou pas assez ?
4. `fold_sweep.wav` : à certains réglages la fondamentale s'efface presque (le son devient creux). Normal pour un
   folder, mais est-ce gênant autour du réglage par défaut (0,3) ?
5. `fold_bias_sweep.wav` : BIAS change-t-il nettement la couleur ? Le milieu (BIAS à 0) est-il le plus « plein » ?
6. `tzfm_sweep.wav` : la note reste-t-elle bien la même pendant que le timbre s'enrichit ? La fin est-elle assez
   métallique ?
7. `vcf_res_sweep.wav` : la résonance à fond est-elle assez marquée sans siffler ? Les graves tiennent-ils ?
8. `mixer_drive.wav` : la saturation du mixeur est-elle musicale ? Assez « crade » en butée ?
9. `order_compare.wav` : les trois modes sont-ils nettement différents ?
10. `alias_fold_high.wav` : entends-tu des sons parasites graves ou « numériques » sous la note aiguë ? Si oui, le
    mode ×8 passera en priorité en phase 6.

## Checkpoint phase 4 — Patch bay

Mêmes commandes de rendu. Il n'y a pas encore de câbles à l'écran (phase 5) : ces rendus sont patchés par le code.
Spectrogrammes : `docs/checkpoints/phase-4/`.

| Fichier | Câbles | Ce qu'on doit entendre |
|---|---|---|
| `patch_bitflip_lfo.wav` | M VCO (0,7 Hz) → BIT FLIP 1 | Le rythme de SEQ1 se réécrit en continu |
| `patch_clock2_polymeter.wav` | M VCO (3 Hz) → CLOCK 2 | SEQ2 à 3 pas par seconde contre 4 pour SEQ1 |
| `patch_fm_1voct.wav` | M VCO → U MIX 1, U MIX 1+2 → VCO 1V/OCT | FM exponentielle, dosée par U MIX 1 LVL de 0 à 0,3 |
| `patch_noise_cutoff.wav` | NOISE → CUTOFF | Le bruit module le filtre, de plus en plus fort |
| `patch_eg1_vcw_vca.wav` | EG1 → VCW VCA CV | Claquement court du folder sur la longue queue du filtre |
| `patch_feedback.wav` | VCA → VCF IN | La sortie revient dans le filtre |

Questions :
1. `patch_bitflip_lfo.wav` : le rythme change-t-il de façon vivante, ou trop vite pour rester musical ?
2. `patch_clock2_polymeter.wav` : entends-tu bien deux tempos indépendants (4 contre 3) ?
3. `patch_fm_1voct.wav` : la FM monte-t-elle progressivement ? Est-ce un son utile, ou faut-il un atténuateur plus
   fin que U MIX 1 LVL ?
4. `patch_noise_cutoff.wav` : la modulation du filtre par le bruit donne-t-elle un grain intéressant ?
5. `patch_eg1_vcw_vca.wav` : les deux decays sont-ils bien distincts (attaque courte, queue longue) ?
6. `patch_feedback.wav` : le feedback apporte-t-il du caractère sans devenir incontrôlable ?
7. Avec un câble dans EG2 TRIG, j'ai supposé que le VCA ne suit plus les triggers des séquenceurs, seulement ce
   jack. Est-ce le comportement que tu attends ?
8. Faut-il exposer des jacks à la DAW (sorties CV, entrée sidechain) ? La spec le prévoit en option en phase 6.

## Checkpoint phase 5 — Interface

À essayer dans Ableton (le preset `debug` installe le plugin). Capture : `docs/checkpoints/phase-5/panneau.png`.

1. Les knobs se règlent-ils bien à la souris (sensibilité, double-clic pour revenir à la valeur par défaut) ? La
   valeur affichée pendant le réglage est-elle lisible ?
2. Les petits réglages (EG1, SEQ, FM...) sont-ils assez gros pour être attrapés à 100 % ? À quelle taille de
   fenêtre travailles-tu ?
3. RUN / STOP, puis BIT FLIP en cours de lecture : le pas sous le write head s'inverse-t-il au clic ? En tenant le
   bouton, toute la séquence s'inverse-t-elle ?
4. Combinaisons avec SHIFT : allumer SHIFT, cliquer BIT SHIFT 1 (il reste allumé), puis BIT FLIP 1 plusieurs fois.
   Le quantizer change-t-il de mode, et les 16 LEDs montrent-elles le mode ? Ce fonctionnement est-il clair, ou
   préfères-tu autre chose (un menu sur l'afficheur, par exemple) ?
5. BUFFER tenu une seconde, puis CORRUPT à fond quelques secondes, puis BUFFER bref : la séquence revient-elle ?
6. Câbles : glisser de M VCO vers BIT FLIP 1, déplacer le câble vers BIT FLIP 2, le retirer par double-clic. Le
   geste est-il naturel ? Les couleurs par type de signal (ambre audio, bleu CV, ivoire trigger) te conviennent-elles ?
7. L'oscilloscope est-il utile à cette taille ? À garder, agrandir ou retirer ?
8. Lisibilité générale : libellés, contraste, densité. Le module Wavefolder est un peu vide : je resserre, ou tu
   veux y voir autre chose ?
9. L'habillage final (Chakra Petch, rouge, pans coupés) est-il bien ce que tu voulais une fois dans la DAW ?
