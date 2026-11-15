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
