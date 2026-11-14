# DÉDALE — Spécification pour Claude Code

> **Rôle** : tu es un·e développeur·se DSP/JUCE senior. Tu vas construire **DÉDALE**, un instrument VST3/AU qui émule fidèlement le *comportement* et le *son* d'un synthétiseur semi-modulaire analogique génératif « parallèle » (architecture décrite ci-dessous, inspirée du Moog Labyrinth, 2024).
>
> **Règle de design non négociable** : l'interface reprend la **même logique de sections et la même ergonomie** (ce qu'on contrôle, où c'est groupé, patch bay) mais avec une **identité visuelle originale**. Pas de logo, nom, typographie, flancs en bois, couleurs ou sérigraphie de Moog/Labyrinth, nulle part (UI, code, métadonnées, presets). Le produit s'appelle DÉDALE.
>
> Lis tout ce document avant d'écrire du code. Le **processus de développement à suivre est en section 15** : il prime sur tes habitudes. Commence par la phase 0 (mise en place du repo, `CLAUDE.md`, docs), puis avance phase par phase (section 13). Le projet comprend aussi un **tutoriel web interactif** (section 16).

---

## 0. Résumé de l'instrument

Synthé **monophonique** à une voix, sans clavier, piloté par **deux séquenceurs génératifs 8 pas de type registre à décalage** (« bits »). Une source sonore (VCO sinus + MOD VCO triangle + ring mod + bruit → mixeur saturant) alimente **deux chemins de traitement** : un **wavefolder (VCW)** et un **filtre state-variable 2 pôles (VCF)**, chacun suivi de son propre VCA. Les deux chemins sont en **parallèle** ou en **série** (dans un sens ou l'autre), puis mélangés par un crossfader **BLEND**. Deux enveloppes **decay-only**. Une **patch bay virtuelle de 32 points** (20 entrées, 12 sorties) avec des connexions normalisées.

Caractère sonore à viser : plutôt « West Coast » que Moog classique. Agressif, métallique, percussif, très riche en harmoniques dès qu'il y a de la FM et du folding. Le sinus et le triangle seuls sonnent propres. Tout l'intérêt est dans la FM through-zero, le folding et la saturation du mixeur.

---

## 0bis. Documentation de référence (manuels officiels)

Ces manuels sont la **source de vérité** pour le comportement des machines d'origine. Ce document en est un résumé : en cas de doute ou de contradiction (en particulier sur les points de la section 14), **va vérifier dans le manuel**, puis note ce que tu as trouvé dans `docs/DECISIONS.md`.

| Instrument LABRYS | Machine de référence | Manuel / doc |
|---|---|---|
| DÉDALE | Moog Labyrinth | https://www.analoguehaven.com/moog-music/labyrinth/manual.pdf |
| TALOS | Moog DFAM | https://cn.juno.co.uk/downloads/manual/675126-01U.pdf |
| ICARE | Moog Subharmonicon | https://analoguehaven.com/moog-music/subharmonicon/manual.pdf |
| TALOS + ICARE | Patch book DFAM & Subharmonicon (site Moog) | https://back.moogmusic.com/sites/default/files/2021-04/DFAM&SUBH%20patch%20book.pdf |
| EUROPE | Moog Grandmother | https://analoguehaven.com/moog-music/grandmother/manual.pdf |
| EUROPE | Réglages globaux Grandmother (site Moog) | https://back.moogmusic.com/sites/default/files/2021-06/Grandmother_Global_Settings.pdf |
| PHÈDRE | Moog Mavis (manuel en ligne) | https://www.manualslib.com/manual/2603954/Moog-Mavis.html |
| PHÈDRE | Guide « Patching with Intention » Mavis | https://www.manualslib.com/manual/2910548/Moog-Mavis.html |

Pages produit officielles (specs, éventuelles nouvelles versions des manuels) : https://www.moogmusic.com/products/subharmonicon, https://www.moogmusic.com/products/mavis, https://www.moogmusic.com/products/semi-modular/grandmother. Si une version plus récente d'un manuel existe sur moogmusic.com, elle prime.

**Comment les utiliser :**
- Télécharge les PDF dans `docs/reference/` et **ajoute ce dossier au `.gitignore`** : ces manuels appartiennent à Moog, ils servent de référence locale, on ne les redistribue pas.
- Extrais le texte avec `pdftotext -layout` pour pouvoir y chercher (`grep`), et regarde les pages de schémas (flux du signal, patch bay) en image quand c'est utile.
- Si ton accès réseau refuse un téléchargement, ne cherche pas de contournement : demande à Gaby de déposer le PDF dans `docs/reference/`.
- Tu peux t'en servir pour comprendre le fonctionnement, mais **ne recopie ni texte, ni illustration, ni schéma** des manuels dans le code, l'UI, la doc du repo ou le tutoriel web : tout doit être rédigé et dessiné à neuf.
- Pour chaque instrument suivant (TALOS, ICARE, EUROPE, PHÈDRE), commence par rédiger une spec au même format que ce document (`docs/SPEC_<NOM>.md`) à partir de son manuel, et fais-la valider par Gaby avant de coder.

---

## 1. Stack technique

- **JUCE 8** (dernière version stable), **CMake** (pas Projucer), **C++20**.
- Formats : **VST3, AU, Standalone**. macOS universal (arm64 + x86_64) en priorité, Windows ensuite.
- Type de plugin : **instrument** (synth), entrée MIDI, sortie stéréo (signal mono dupliqué). Une entrée audio sidechain optionnelle (voir §8, jacks VCW IN / VCF IN / U MIX).
- `AudioProcessorValueTreeState` pour tous les paramètres automatisables.
- Tests : **Catch2** (ou GoogleTest) dans une cible séparée `DedaleTests`.
- Aucune dépendance propriétaire. Code DSP 100 % maison, isolé de JUCE autant que possible (`/Source/DSP/` sans include JUCE sauf `juce::dsp::Oversampling` si besoin).

---

## 2. Flux du signal

```
             ┌──────────── EG1 (decay) ─────────────┐ (vers VCO, MOD VCO, FOLD, CUTOFF via AMT bipolaires)
SEQ1 ──CV──► VCO pitch (QTZ) ; VCW FOLD
SEQ2 ──CV──► MOD VCO pitch (QTZ) ; VCF CUTOFF
SEQ1/2 TRIG ─► EG TRIG MIX ─► EG1 + EG2

 VCO (sinus) ◄── TZFM ── MOD VCO (triangle)
   │   └──► RING MOD ◄──┘           NOISE (tone)
   ▼          ▼              ▼          ▼
 ┌───────────── MIXER SATURANT (4 canaux) ─────────────┐
 └──────────────────────┬──────────────────────────────┘
                        ▼
          ORDER : PARALLEL | VCW>VCF | VCF>VCW
         ┌──────────────┴──────────────┐
         ▼                             ▼
   VCW (wavefolder)              VCF (SVF LP↔BP)
         ▼                             ▼
   VCA VCW (EG2)                 VCA VCF (EG2)
         └────────► BLEND (crossfade, CV) ◄───┘
                        ▼
                     VOLUME ─► sortie
  U MIX : (U MIX 1 [normalisé RING MOD] × LVL) + U MIX 2 → sortie jack U MIX 1+2
```

Détail ORDER :
- **PARALLEL** : VCW et VCF reçoivent tous deux la sortie du mixeur.
- **VCW>VCF** : l'entrée du VCF vient de la sortie du VCW (avant son VCA).
- **VCF>VCW** : l'entrée du VCW vient de la sortie du VCF (avant son VCA).
- Dans tous les cas, chaque chemin garde son VCA, puis BLEND mélange les deux sorties de VCA.

**Convention interne** : toutes les tensions de modulation sont des `float` exprimés **en volts** (1.0f = 1 V), comme en Eurorack. Audio : ±5 V internes, normalisés en ±1.0 à la sortie (`out = v / 5.0f * headroom`). Ça rend la patch bay triviale et cohérente.

---

## 3. Sources

### 3.1 VCO (sinus)
- Plage : **~20 Hz → ~5 kHz** (knob `VCO FREQUENCY`, courbe exponentielle).
- Entrée 1V/oct (jack `VCO 1V/OCT`, ±5 V) sommée au knob.
- `VCO EG1 AMT` : **bipolaire**, module la hauteur par EG1.
- `VCO SEQ1 AMT` : unipolaire 0 → max. **À la butée max (cran « QTZ »), la tension SEQ1 passe à gain unité en 1V/oct, donc les notes sont justes et quantifiées.** En dessous, c'est une version atténuée (donc « fausse », exploitable pour des glissements ou du désaccord). Implémente le cran QTZ comme une zone des derniers ~3 % de course qui verrouille le gain à exactement 1.0.
- **FM through-zero (TZFM)** depuis le MOD VCO, réglée par `MOD VCO FM AMT` (le MOD VCO est normalisé sur l'entrée FM du VCO). TZFM **linéaire** : la fréquence instantanée peut devenir négative, l'accumulateur de phase recule. Le pitch perçu du VCO reste centré quand on monte l'index (c'est le but du TZ). Implémentation : `phaseInc = f0 * (1 + index * mod) / sr`, phase wrap dans les deux sens.
- Anti-aliasing : le sinus pur n'en a pas besoin, mais la TZFM oui. Prévois un oversampling de la section source (voir §9).
- Option « analog drift » : très léger bruit lent sur le pitch (désactivable), pour éviter l'aspect trop clinique.

### 3.2 MOD VCO (triangle)
- Plage : **de l'ordre de quelques secondes par cycle (LFO) → ~1,3 kHz**. Courbe exponentielle, couvrir ~0,1 Hz → 1,3 kHz.
- Entrée `M VCO 1V/OCT` (±5 V), entrée `M VCO SYNC` (gate : **reset de phase tant que le gate est haut**, pas seulement sur le front).
- `MOD VCO EG1 AMT` : bipolaire.
- `MOD VCO SEQ2 AMT` : même logique QTZ que le VCO, mais avec SEQ2.
- Sortie jack `M VCO` (±5 V). Utilisation typique : en LFO lent vers BIAS, BLEND ou les entrées BIT FLIP.
- Triangle anti-aliasé (PolyBLAMP ou intégration d'un carré BLEP).

### 3.3 RING MOD
- Produit VCO × MOD VCO. Niveau `RING MOD LVL` dans le mixeur. Sortie normalisée sur l'entrée `U MIX 1`.

### 3.4 NOISE
- Générateur de bruit à **tonalité variable** : `NOISE TONE` va du grave (gauche) à l'aigu (droite). Implémentation suggérée : bruit blanc → filtre passe-bande ou tilt dont la fréquence centrale balaie ~100 Hz → ~12 kHz. Le bruit n'est jamais vraiment « blanc » sur le hardware.
- Niveau `NOISE LVL`. Sortie jack `NOISE`.

### 3.5 MIXER saturant
- 4 canaux : `VCO LVL`, `RING MOD LVL`, `MOD VCO LVL`, `NOISE LVL`.
- **Gain unité à midi, saturation au-delà.** Chaque canal sature individuellement, puis la somme sature à nouveau légèrement.
- Modèle : `ch = tanh(g * x) / normalisation`, avec `g` qui monte vers ~4–6 en butée. Asymétrie légère (offset DC avant tanh puis DC-block) pour des harmoniques paires. Le mixeur poussé dans le filtre doit sonner « crade » et c'est voulu.
- Sortie jack `MIXER` (avant VCW/VCF).

---

## 4. Wavefolder (VCW)

- Hardware : wavefolder hybride diode-transistor conçu pour **adoucir les points de cassure** audibles. Doit sonner plus **propre et moins symétrique** qu'un folder « Buchla 259 » brut.
- `VCW FOLD` : propre au minimum, folding max au maximum.
- `VCW BIAS` : offset DC ajouté avant le folder. Au-dessus de midi positif, en dessous négatif. Déplace la symétrie et donc l'équilibre harmoniques paires/impaires.
- `VCW EG1/CV AMT` : bipolaire. Atténuverteur de EG1 vers FOLD, ou de la CV externe si le jack `VCW FOLD` est patché (le jack est **normalisé sur EG1**).
- `VCW SEQ1 AMT` : envoie la CV bipolaire de SEQ1 (après CV RANGE et quantizer) vers FOLD.
- Entrée directe `VCW IN` (audio ±5 V) qui remplace la source de l'étage.

**Modèle DSP proposé** (à comparer à l'oreille, garde les variantes derrière un enum interne) :
1. Pré-gain `1 + fold * k` (k ~ 8–10), + bias.
2. Deux étages de folding doux successifs (ex. : folder de Lockhart/Esqueda simplifié, ou `sin()`-fold mélangé à un `tanh`). Les rampes de pli doivent paraître un peu « en escalier » sur un oscilloscope à fort FOLD.
3. Saturation finale douce, puis DC-block.
4. **Coloration résiduelle** : même à FOLD = 0, le hardware colore légèrement le signal. Ajoute un paramètre caché `vcwZeroColour` (léger tanh, ~0,1–0,2 de drive), activé par défaut.
5. Anti-aliasing : **ADAA du 1er ordre** si la forme le permet, sinon oversampling ×4 minimum (×8 en mode « HQ »).

---

## 5. Filtre (VCF)

- **SVF 2 pôles (12 dB/oct)**, pas un ladder. **Pas de perte de graves quand la résonance monte.**
- `VCF CUTOFF` : ~20 Hz → 20 kHz, exponentiel. Jack `CUTOFF` (±5 V, 1V/oct approx.) **normalisé sur EG1**.
- `VCF EG1/CV AMT` : bipolaire, atténuverteur EG1 ou CV externe.
- `VCF SEQ2 AMT` : CV de SEQ2 vers cutoff.
- `RESONANCE` : de zéro à **presque** l'auto-oscillation. **Ne doit pas auto-osciller** (résonance « bien élevée », un peu retenue).
- `FILTER MODE` : **morph continu LP (gauche) → BP (droite)**, avec mélanges intermédiaires (`out = (1-m)*LP + m*BP`, avec compensation de niveau du BP).
- Implémentation : **TPT / SVF de Zavalishin ou Simper (Cytomic)**, non linéarité douce (tanh) dans la boucle d'intégrateur pour le grain quand le mixeur pousse fort. Modulation de cutoff à l'audio-rate sans clic (calcul des coefficients par échantillon ou par petit bloc interpolé).
- Entrée directe `VCF IN` (audio ±5 V).

---

## 6. VCAs, BLEND, sortie, U MIX

- **Un VCA par chemin** (VCW et VCF). Les deux sont pilotés par **EG2** par défaut (jacks `VCW VCA CV` et `VCF VCA CV`, 0 → +8 V, **normalisés sur EG2**). Patcher EG1 sur un des VCA donne un decay indépendant par chemin.
- Réponse des VCA : légèrement exponentielle (courbe à ajuster), 0 V = silence total.
- `BLEND` : crossfader **équipuissance** entre la sortie VCA-VCW et la sortie VCA-VCF. Knob + jack `BLEND` (±5 V sommé).
- `VOLUME` : niveau de sortie finale (et du jack virtuel `VCA`).
- **U MIX** (mixeur utilitaire DC-coupled) : `U MIX 1` (normalisé sur RING MOD) passe par `U MIX 1 LVL`, `U MIX 2` entre à gain unité. Sortie jack `U MIX 1+2`. Ça sert de mixeur de CV aussi bien que d'audio.
- Sortie plugin : `VCA` (somme BLEND × VOLUME), soft-clip de sécurité, DC-block, puis ×0,2 pour revenir en ±1.0.

---

## 7. Enveloppes

- **EG1 et EG2 : decay seulement, pas d'attaque.** `EG1 DECAY` et `EG2 (VCA) DECAY` : **~10 ms → 5–10 s** (le manuel dit « plus de 5 s », des mesures de possesseurs suggèrent plutôt ~10 s : mets 10 s en butée, courbe de knob exponentielle). EG1 et EG2 ont exactement le même comportement à réglage égal.
- Forme : décroissance exponentielle (RC), sortie 0 → +8 V.
- **Anti-clic** : micro-attaque de ~0,5–1 ms (paramètre caché) + léger lissage (one-pole ~2 kHz) sur la CV de sortie. La source principale de clics vient des **sauts de vélocité** entre triggers successifs, pas de l'attaque : à chaque retrigger, l'enveloppe repart du niveau courant vers la nouvelle crête avec un slew très court plutôt que de sauter.
- **EG TRIG MIX** (crossfade de vélocité des triggers) :
  - butée gauche : triggers SEQ1 à vélocité 100 %, SEQ2 muet ;
  - butée droite : SEQ2 à 100 %, SEQ1 muet ;
  - midi : les deux à niveau égal.
  - La « vélocité » = hauteur de crête de l'enveloppe. Si SEQ1 et SEQ2 tombent sur le même pas, prends le max.
- `TRIGGER` (bouton + jack) : déclenche **les deux EG** à pleine vélocité. Si le jack `EG2 TRIG` est patché, `TRIGGER` ne déclenche plus que EG1.
- Jacks de sortie `EG1`, `EG2` (0 → +8 V).

---

## 8. Séquenceur génératif double (le cœur du projet)

Implémente-le dans une classe **pure** `GenerativeSequencer` (aucun JUCE, RNG injectable et seedable pour les tests), avec deux instances `SeqLane` et un objet de coordination pour le chain/clock.

### 8.1 Modèle de données (par lane)
```cpp
struct SeqLane {
    std::array<bool, 8>  bit;        // pas actif (on/off)
    std::array<float, 8> voltage;    // tension stockée, -5..+5 V
    int length = 8;                  // 1..8
    int playHead = 0;                // position de lecture
    int writeOffset = 0;             // décalage write head par rapport au play head (0..7)
    float corrupt = 0.f;             // 0..1
    float cvRange = 1.f;             // 0..1 (≈ 1 à 10 octaves)
};
```
- **Write head** = `(playHead + writeOffset) % length`. Par défaut offset = 0, donc le write head suit le play head. Dans l'UI : LED verte = play head, LED rouge qui clignote en rythme = write head, LED rouge fixe = bit actif.

### 8.2 À chaque tick d'horloge (ordre à respecter)
1. Avancer `playHead = (playHead + 1) % length`.
2. **Corrupt** évalué sur le pas du **write head** :
   - `corrupt` dans [0, 0.5] : probabilité `pVolt = lerp(0, 0.25, corrupt/0.5)` de remplacer la tension stockée par une nouvelle valeur aléatoire uniforme (−5..+5 V). Environ 25 % à midi.
   - `corrupt` dans ]0.5, 1] : `pVolt` continue jusqu'à ~0.5 en butée, **et** une probabilité `pFlip = lerp(0, 0.5, (corrupt-0.5)/0.5)` de **flipper le bit** est ajoutée.
   - Un bit qui passe de off à on reçoit **toujours une nouvelle tension aléatoire**. Un bit qui passe à off garde sa tension en mémoire mais ne la sort plus.
   - À corrupt = 0, la séquence se répète **exactement**.
   - *(Hypothèse d'implémentation à valider à l'écoute : appliquer corrupt avant de lire le pas, pour qu'une mutation sur le pas courant s'entende tout de suite quand offset = 0.)*
3. Lire le pas du **play head** : si `bit[playHead]` est on, alors **trigger** (1 ms, 0 → +5 V) et la CV de sortie prend `voltage[playHead]`. Si le bit est off, pas de trigger et **la CV garde sa dernière valeur** (sample & hold).
4. CV de sortie = `voltage × cvRange` (CV RANGE ≈ 1 à 10 octaves : ±0,5 V → ±5 V), **puis quantizer**.
5. Option globale « polarité CV » : bipolaire ±5 V (défaut) ou unipolaire 0 → +5 V (`v*0.5 + 2.5`).

### 8.3 Actions (boutons et jacks)
| Action | Effet |
|---|---|
| `BIT FLIP n` (bouton ou front montant sur le jack `BIT FLIP n`) | Inverse le bit au **write head**. off→on : nouvelle tension aléatoire ±5 V. |
| Maintien `BIT FLIP n` pendant la lecture | Flippe **chaque** bit au passage du write head (inversion complète de la séquence). |
| `LENGTH n` | Longueur −1 à chaque appui (8→7→…→1), puis retour à 8. |
| `BIT SHIFT n` | Rotation de la séquence d'un pas vers la droite (le dernier pas actif revient au pas 1), **dans la longueur courante**. Tourne bits ET tensions ensemble. |
| `CHAIN SEQ` | Fusionne les deux lanes en une seule séquence jusqu'à 16 pas (SEQ1 puis SEQ2, play heads en phase). En mode chain : `BIT SHIFT 1` tourne les deux séquences ensemble, `BIT SHIFT 2` fait avancer **seulement** le play head de SEQ2. |
| `RUN/STOP` | Démarre ou arrête les deux séquenceurs. **Pas de reset à l'arrêt.** |
| `RESET` (bouton/jack) | Play head au pas 1, en **conservant** l'offset du write head. |
| `ADVANCE` | Avance play head et write head d'un pas. **Inactif pendant la lecture.** |
| `BUFFER` maintenu 1 s | Sauvegarde : bits, tensions, mode quantizer, offsets du write head, longueurs, mode chain. **Un seul slot.** |
| `BUFFER` appui court | Recharge le buffer. |

### 8.4 Combinaisons (à reproduire telles quelles, en plus d'un bouton `SHIFT` dédié dans l'UI)
- `LENGTH + RESET` : longueur remise à 8.
- `BIT SHIFT + RESET` : remet les bits à leur position d'origine (annule les rotations : garde un compteur de rotation).
- `BIT SHIFT + ADVANCE` : décale le write head d'un pas vers l'avant.
- `BIT SHIFT + ADVANCE + RESET` : write head ramené sur le play head.
- `BIT SHIFT 1 + BIT FLIP 1` : quantizer, mode suivant.
- `BIT SHIFT 1 + LENGTH 1` : quantizer, mode précédent.
- `BIT SHIFT 2 + ADVANCE` (en mode chain) : avance le play head de SEQ2 d'un pas.
- `BIT SHIFT 2 + RESET` (en mode chain) : recale le play head de SEQ2 sur celui de SEQ1.
- `BUFFER + RESET` maintenus 1 s : efface la séquence (bits, quantizer, offsets).
- `BIT SHIFT 2 + LENGTH 2` / `BIT SHIFT 2 + BIT FLIP 2` : division d'horloge MIDI/hôte −/+ (1 à 16).
- Pendant un réglage de mode, la rangée de LEDs SEQ affiche le mode courant (1 à 16).

### 8.5 Quantizer (16 modes, défaut = 3 Major)
Quantification au demi-ton le plus proche **dans la gamme active**, root = C par défaut, transposable par MIDI Note On (§8.7). S'applique aussi aux jacks `SEQ1 CV` / `SEQ2 CV`.

| # | Mode | Degrés (demi-tons depuis la fondamentale) |
|---|---|---|
| 1 | Non quantifié | — |
| 2 | Chromatique | 0 1 2 3 4 5 6 7 8 9 10 11 |
| 3 | Majeur | 0 2 4 5 7 9 11 |
| 4 | Pentatonique | 0 2 4 7 9 |
| 5 | Mineur mélodique | 0 2 3 5 7 9 11 |
| 6 | Mineur harmonique | 0 2 3 5 7 8 11 |
| 7 | Diminished 6th | 0 2 4 5 7 8 9 11 *(à vérifier)* |
| 8 | Par tons | 0 2 4 6 8 10 |
| 9 | Hirajoshi | 0 2 3 7 8 *(à vérifier)* |
| 10 | 7sus4 (1 4 5 b7) | 0 5 7 10 |
| 11 | Maj7 (1 3 5 7) | 0 4 7 11 |
| 12 | Maj13 (1 3 5 6 7 9) | 0 2 4 7 9 11 |
| 13 | Min7 (1 b3 5 b7) | 0 3 7 10 |
| 14 | Min11 (1 b3 4 5 b7 9) | 0 2 3 5 7 10 |
| 15 | Accordage handpan | 0 2 3 5 7 8 10 *(hypothèse : gamme type « Kurd », à vérifier)* |
| 16 | Quads (tierces mineures) | 0 3 6 9 |

Mets ces gammes dans une table `constexpr` facile à éditer. Les lignes marquées *(à vérifier)* sont des interprétations : laisse un `// TODO calibrate` dessus.

### 8.6 Horloge
- Horloge interne : knob `TEMPO` (propose ~10 → 600 pas/min, exponentiel, affichage en BPM et en Hz).
- **Sync hôte** (remplace le MIDI clock du hardware) : quand le transport de la DAW tourne et que `SYNC = HOST`, dérive l'horloge des PPQ de `AudioPlayHead` avec la division réglable (1 à 16, défaut 1/16). Calcul **sample-accurate** dans le bloc.
- Jacks `CLOCK 1` (normalisé sur l'horloge interne/hôte) et `CLOCK 2` (normalisé sur `CLOCK 1`) : patcher une source dans `CLOCK 2` (ex. `M VCO` lent) donne une **horloge indépendante à SEQ2**, donc de la polymétrie.
- Priorité : jack externe > hôte/MIDI > interne. Retour à l'interne 5 s après la dernière impulsion MIDI clock (en standalone).
- Messages MIDI (standalone et hôte) : Start = reset + run, Stop = stop, Continue = run sans reset, Song Position 0 = reset.
- Sortie jack `CLOCK` (0 → +5 V).

### 8.7 MIDI
- **Note On avec quantizer actif** : transpose la fondamentale des CV quantifiées de SEQ1 et SEQ2 (note − 60 = décalage en demi-tons, réglable).
- **Note On séquenceurs à l'arrêt** : déclenche un trigger (on joue DÉDALE comme un instrument à percussion).
- Canal MIDI : réglage global (défaut omni).
- **Bonus plugin** : option « MIDI out » qui émet les triggers de SEQ1/SEQ2 en notes MIDI (hauteur = CV quantifiée), pour enregistrer les séquences générées dans la DAW.

### 8.8 Réglages globaux (menu engrenage dans l'UI, persistés dans le state)
- Comportement du jack RESET : retour au pas 1 (défaut) / retour au pas 1 + rappel du buffer.
- Polarité des CV SEQ : bipolaire (défaut) / unipolaire.
- Qualité d'oversampling, drift analogique, coloration du VCW à zéro.

### 8.9 Déterminisme et tests
- RNG : `xoshiro128**` ou `std::mt19937`, seed exposée pour les tests, seed aléatoire en usage normal.
- Tests unitaires obligatoires : corrupt = 0 → séquence identique sur 1000 cycles ; BIT SHIFT + rotation inverse = identité ; LENGTH qui boucle 8→1→8 ; offset write head ; chain 16 pas ; statistiques de corrupt (pVolt ≈ 0,25 à 0,5 ; pFlip ≈ 0,5 à 1,0, sur 100 000 tirages, tolérance ±2 %) ; quantizer (chaque mode, valeurs limites) ; save/recall du buffer.

---

## 9. Patch bay virtuelle (32 points)

Implémente un **routeur de modulation** : chaque jack est un `float` (en volts) par échantillon. Un câble relie une sortie à une entrée, une sortie peut alimenter plusieurs entrées, une entrée n'accepte qu'un câble. **Patcher une entrée casse sa normalisation.** Les câbles audio-rate sont évalués **par échantillon** (pas par bloc) pour que la FM, le sync et le bit flip audio-rate fonctionnent. Gère les boucles de feedback avec un délai d'un échantillon.

**Entrées (20)**
| Jack | Plage | Normalisé sur |
|---|---|---|
| VCO 1V/OCT | ±5 V | — (sommé au knob) |
| M VCO SYNC | gate 0/+5 V | — (reset de phase tant que haut) |
| M VCO 1V/OCT | ±5 V | — |
| BLEND | ±5 V | — (sommé) |
| VCW IN | audio ±5 V | sortie mixeur / ORDER |
| VCW FOLD | ±5 V | **EG1** (via EG1/CV AMT) |
| VCW VCA CV | 0..+8 V | **EG2** |
| VCF IN | audio ±5 V | sortie mixeur / ORDER |
| CUTOFF | ±5 V | **EG1** (via EG1/CV AMT) |
| VCF VCA CV | 0..+8 V | **EG2** |
| U MIX 1 | ±5 V DC | **RING MOD** |
| U MIX 2 | ±5 V DC | — |
| EG2 TRIG | trig, front montant | — |
| CLOCK 1 | trig | horloge interne/hôte |
| BIT FLIP 1 | trig, front montant | — |
| CLOCK 2 | trig | **CLOCK 1** |
| BIT FLIP 2 | trig, front montant | — |
| MIDI | — | représenté visuellement, câblé sur l'entrée MIDI du plugin |
| TRIGGER | trig | — (EG1+EG2, ou EG1 seul si EG2 TRIG patché) |
| RESET | trig | — |

**Sorties (12)** : `VCA` (sortie principale), `M VCO`, `NOISE`, `MIXER`, `EG1`, `EG2`, `U MIX 1+2`, `SEQ1 CV`, `SEQ1 TRIG`, `SEQ2 CV`, `SEQ2 TRIG`, `CLOCK`.

Seuil des entrées trigger/gate : front montant au-dessus de +1 V avec hystérésis (redescente sous +0,5 V).

**Extensions plugin** (facultatives, phase 6) : 4 jacks « HOST » exposés comme paramètres automatisables (macro-CV pilotables depuis la DAW) et 1 entrée sidechain audio.

---

## 10. Paramètres (APVTS)

IDs en `snake_case` stables (ne jamais les renommer après la v1).

| ID | Nom UI | Plage | Défaut |
|---|---|---|---|
| vco_freq | VCO FREQUENCY | 20 Hz–5 kHz (skew exp.) | ~110 Hz |
| vco_eg1_amt | VCO EG1 AMT | −1..+1 | 0 |
| vco_seq1_amt | VCO SEQ1 AMT | 0..1 (cran QTZ en butée) | 1 (QTZ) |
| mvco_freq | MOD VCO FREQUENCY | 0,1 Hz–1,3 kHz | ~55 Hz |
| mvco_eg1_amt | MOD VCO EG1 AMT | −1..+1 | 0 |
| mvco_seq2_amt | MOD VCO SEQ2 AMT | 0..1 (QTZ) | 0 |
| mvco_fm_amt | FM AMT (TZFM) | 0..1 | 0 |
| mix_vco | VCO LVL | 0..1 (unité à 0,5) | 0,5 |
| mix_ring | RING MOD LVL | 0..1 | 0 |
| mix_mvco | MOD VCO LVL | 0..1 | 0 |
| mix_noise | NOISE LVL | 0..1 | 0 |
| noise_tone | NOISE TONE | 0..1 | 0,5 |
| vcw_fold | VCW FOLD | 0..1 | 0,3 |
| vcw_bias | VCW BIAS | −1..+1 | 0 |
| vcw_eg1_amt | VCW EG1/CV AMT | −1..+1 | 0,3 |
| vcw_seq1_amt | VCW SEQ1 AMT | 0..1 | 0 |
| vcf_cutoff | VCF CUTOFF | 20 Hz–20 kHz | 800 Hz |
| vcf_eg1_amt | VCF EG1/CV AMT | −1..+1 | 0,3 |
| vcf_seq2_amt | VCF SEQ2 AMT | 0..1 | 0 |
| vcf_res | RESONANCE | 0..1 | 0,2 |
| vcf_mode | FILTER MODE (LP→BP) | 0..1 | 0 |
| order | ORDER | PARALLEL / VCW>VCF / VCF>VCW | PARALLEL |
| blend | BLEND | 0..1 | 0,5 |
| eg1_decay | EG1 DECAY | 10 ms–10 s | 300 ms |
| eg2_decay | EG2 DECAY | 10 ms–10 s | 400 ms |
| eg_trig_mix | EG TRIG MIX | 0..1 | 0 |
| tempo | TEMPO | 10–600 pas/min | 120 |
| sync_mode | SYNC | INTERNAL / HOST | HOST |
| clock_div | DIVISION | 1..16 | 1/16 |
| seq1_corrupt / seq2_corrupt | CORRUPT | 0..1 | 0 |
| seq1_range / seq2_range | CV RANGE | 0..1 | 0,3 |
| umix1_lvl | U MIX 1 LVL | 0..1 | 0 |
| volume | VOLUME | 0..1 | 0,7 |
| run | RUN/STOP | bool | false |
| chain | CHAIN SEQ | bool | false |
| quant_mode | QUANTIZER | 1..16 | 3 |

Lissage : `SmoothedValue` (~20 ms) sur tout ce qui est continu. Pas de lissage sur les booléens/choix.

**Hors APVTS mais dans le state** (`getStateInformation`) : bits, tensions, longueurs, offsets, play heads, buffer, câbles de la patch bay, réglages globaux. **Contrairement au hardware**, la DAW doit retrouver exactement la séquence et le patch à la réouverture du projet. Sérialisation en XML/ValueTree, versionnée (`stateVersion`).

---

## 11. Interface (identité DÉDALE, originale)

**Principe** : même **zonage fonctionnel** qu'un 60HP semi-modulaire (format paysage, rapport ~2,3:1), habillage graphique 100 % nouveau.

### 11.1 Zonage (de gauche à droite)
1. **Bloc séquenceur** (moitié gauche, en haut) : deux rangées de 8 LEDs (SEQ1, SEQ2), chacune avec ses boutons BIT FLIP / BIT SHIFT / LENGTH, knobs CORRUPT et CV RANGE. Au centre : TEMPO, RUN/STOP (gros bouton), RESET, ADVANCE, BUFFER, CHAIN SEQ, TRIGGER, SHIFT.
2. **Bloc sources** (en dessous) : VCO FREQUENCY et MOD VCO FREQUENCY en **gros knobs**, leurs AMT (EG1, SEQ, FM) en **petits trimmers** groupés et étiquetés.
3. **Bloc mixeur** : 4 petits knobs de niveau + NOISE TONE.
4. **Bloc traitement** : VCW (FOLD gros, BIAS, AMT) et VCF (CUTOFF gros, RESONANCE, MODE, AMT) côte à côte, sélecteur ORDER à 3 positions entre les deux, BLEND en dessous.
5. **Bloc enveloppes** : EG1 DECAY, EG2 DECAY, EG TRIG MIX.
6. **Patch bay** (colonne de droite) : grille 4 × 8 de jacks, **sorties visuellement distinguées des entrées** (par exemple un fond inversé derrière les sorties), libellés courts.

### 11.2 Direction artistique (à proposer, pas à copier)
- Thème « labyrinthe / dédale » : façade **gris anthracite mat** ou **béton clair** au choix (deux thèmes), avec un motif de **méandre grec très discret** en filigrane. Accent **cuivre/orange** pour les LEDs et les curseurs de knobs, **cyan** pour le write head.
- Typo : une grotesque condensée libre (ex. *Barlow Condensed* ou *Space Grotesk*, licence OFL, embarquée en BinaryData).
- Knobs dessinés en vectoriel (LookAndFeel custom, pas de filmstrips) : gros knobs à jupe crantée, petits trimmers en « tête de vis » avec indicateur.
- Câbles : courbes de Bézier avec affaissement (« sag »), couleur choisie automatiquement par type de signal (audio / CV / trigger), légère transparence ; glisser depuis un jack pour créer un câble, clic droit ou double-clic pour le supprimer, survol = surbrillance de la source et de la destination.
- LEDs du séquenceur : vert = play head, rouge fixe = bit actif, rouge clignotant = write head (garde cette sémantique, elle est fonctionnelle).
- **Redimensionnable** (75 %–200 %), net en Retina.
- Tooltips sur chaque contrôle (nom + valeur + rôle en une phrase). Affichage de la valeur au survol.
- Petit oscilloscope optionnel (sortie VCA) dans un coin, désactivable.
- Pas de logo Moog, pas de « Labyrinth », pas de flancs en bois, pas de reprise de la sérigraphie d'origine.

### 11.3 Presets
- Format XML dans le dossier utilisateur. Une dizaine de presets d'usine **originaux** (patchs, pas de séquences figées sauf option) : « Sub dub », « Rimshot indus », « Cloche TZFM », « Bruit de forge », « Arpège qui pourrit », « Polymétrie CLOCK 2 », etc.

---

## 12. Conventions de code (à mettre dans CLAUDE.md)

- Arborescence :
  ```
  /Source
    /DSP        (VCO, ModVco, RingMod, Noise, Mixer, Wavefolder, SvfFilter, Vca, DecayEnv, Quantizer)
    /Sequencer  (SeqLane, GenerativeSequencer, Clock, ButtonComboHandler)
    /Patch      (PatchBay, Jack, Normalling)
    /UI         (LookAndFeel, Knob, Trimmer, LedRow, Jack, CableLayer, Panel)
    PluginProcessor.*, PluginEditor.*
  /Tests
  /Resources   (polices, SVG)
  ```
- **Audio thread** : zéro allocation, zéro lock, zéro log. Communication UI↔audio par `std::atomic` et FIFO lock-free (`juce::AbstractFifo`) pour les événements boutons et l'état des LEDs.
- `juce::ScopedNoDenormals` dans `processBlock`.
- Oversampling : section sources + VCW + VCF dans un seul bloc suréchantillonné (×2 éco / ×4 normal / ×8 HQ), séquenceur et enveloppes à la fréquence de base.
- Chaque module DSP : `prepare(sampleRate, maxBlock)`, `reset()`, `processSample(...)`, testé isolément.
- Commits petits et fréquents, messages en français.

---

## 13. Phases et critères d'acceptation

0. **Mise en place** (voir §15.1 à §15.3) : repo git, `CLAUDE.md`, dossier `docs/`, `CMakePresets.json`, CI GitHub Actions qui build et lance les tests (vides au départ). Critère : `cmake --preset debug && cmake --build --preset debug && ctest --preset debug` passe en local et en CI.
1. **Squelette** : projet CMake JUCE, plugin vide qui se charge dans une DAW (Ableton/Reaper) en VST3 et AU, passe `auval` et `pluginval` (niveau 5).
2. **Séquenceur** : `GenerativeSequencer` complet + quantizer + horloge interne/hôte, **tous les tests §8.9 verts**, avant toute UI. Sortie de debug : un bip sinus par trigger pour entendre les patterns.
3. **Voix** : VCO + MOD VCO + TZFM + ring + noise + mixeur + VCW + VCF + VCAs + EGs + BLEND + ORDER. Normalisations par défaut câblées en dur. Critère : un patch « init » sonne directement sans câble (comme sur le hardware, le semi-modulaire doit sonner sans patch).
4. **Patch bay** : routeur par échantillon, normalisations cassables, feedback avec délai d'un échantillon. Tests : patcher `M VCO` → `BIT FLIP 1` flippe à l'audio-rate sans crash, `CLOCK 2` indépendant donne de la polymétrie.
5. **UI** : panneau complet, câbles, LEDs, combos de boutons + bouton SHIFT, redimensionnement, deux thèmes.
6. **Finitions** : state complet, presets, MIDI out, jacks HOST, oversampling réglable, profiling CPU (cible < 5 % d'un cœur M1 à 48 kHz en ×4).
7. **Calibration** : passe d'écoute guidée avec une liste de points à ajuster (courbe du folder, résonance max, saturation du mixeur, courbes des decays, réponse des VCA, tonalité du bruit). Expose ces constantes dans un seul fichier `Calibration.h`.
8. **Tutoriel web** (section 16) : peut démarrer dès que la phase 2 est validée (le séquenceur sert de base aux démos), en parallèle des phases 3 à 7, sur sa propre branche.
9. **Release v1.0** (§15.8).

---

## 14. Points incertains (à traiter comme hypothèses, pas comme vérités)

- Moment exact où CORRUPT agit (avant ou après la lecture du pas) et s'il agit aussi sur les bits off.
- Gammes 7, 9 et 15 du quantizer.
- Courbe exacte du wavefolder (le hardware sonne moins symétrique et plus propre que les modèles classiques).
- Plage max réelle des decays (5 à 10 s).
- Plage exacte du TEMPO interne et du MOD VCO en bas de course.

Pour chacun : implémente la version la plus plausible, isole-la derrière une constante ou un enum dans `Calibration.h`, et signale-la dans ton rapport de fin de phase.

---

## 15. Processus de développement (à suivre à la lettre)

### 15.1 Prérequis et outillage
- macOS : Xcode + Command Line Tools, **CMake ≥ 3.25**, **Ninja**, `git`, `gh`. Vérifie-les au démarrage et liste ce qui manque au lieu de deviner.
- JUCE et Catch2 récupérés via **CMake `FetchContent`** (ou CPM.cmake) avec un **tag de version figé** (jamais `main`).
- `pluginval` (binaire téléchargé dans `tools/`, ignoré par git) et `auval` (fourni par macOS).
- Node.js LTS + pnpm pour le tutoriel web (section 16).
- Identité du plugin : `COMPANY_NAME "Raw Powa Studio"`, `PRODUCT_NAME "DÉDALE"` (affichage) / `Dedale` (fichiers), `PLUGIN_MANUFACTURER_CODE "Rwps"`, `PLUGIN_CODE "Ddle"`, `BUNDLE_ID "com.rawpowastudio.dedale"`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT TRUE`.

### 15.2 Structure du repo
```
/CMakeLists.txt
/CMakePresets.json        (debug, release, tests, asan)
/Source  /Tests  /Resources
/tools                    (scripts : render, analyse, pluginval)
/tutorial                 (site web, section 16)
/docs
  ARCHITECTURE.md         (schéma des classes, flux audio, threads)
  DECISIONS.md            (journal des choix techniques, format ADR court)
  CALIBRATION.md          (chaque constante de Calibration.h : valeur, pourquoi, comment l'écouter)
  PROGRESS.md             (état d'avancement, tâche en cours, prochaines étapes)
  LISTENING_CHECKLIST.md  (ce que Gaby doit écouter à chaque checkpoint)
CHANGELOG.md  README.md  CLAUDE.md  .gitignore  .clang-format
/.github/workflows/ci.yml
```

### 15.3 CLAUDE.md
Le fichier doit contenir, en moins de 150 lignes : le résumé du projet (§0), les conventions (§12), les commandes exactes de build/test/render, les règles du processus (15.4 à 15.7), et un pointeur vers ce document de spec (à copier dans `docs/SPEC.md`). Mets-le à jour dès qu'une convention change.

### 15.4 Git
- `main` = toujours vert (build + tests). Jamais de commit direct dessus.
- Une branche par phase : `phase-2-sequencer`, `phase-3-voice`, etc. Sous-branches si une phase est grosse.
- Commits atomiques au format **Conventional Commits en français** : `feat(seq): ajoute la probabilité de bit flip de CORRUPT`, `fix(vcf): supprime le clic à la modulation de cutoff`, `test(quant): couvre les 16 gammes`.
- Fin de phase : PR via `gh pr create` avec description (ce qui est fait, tests, points à écouter, hypothèses ouvertes), CI verte, puis **attente de la validation de Gaby** avant merge (squash).
- Tag `v0.N.0` à chaque phase mergée.

### 15.5 Boucle de travail pour chaque tâche
1. **Relire** la section de spec concernée + `PROGRESS.md`.
2. **Planifier** : écrire dans `PROGRESS.md` la tâche, les fichiers touchés et les tests prévus (5 à 10 lignes max).
3. **Tests d'abord** pour toute logique pure (séquenceur, quantizer, horloge, routeur, sérialisation) : écrire le test, le voir échouer.
4. **Implémenter** le minimum pour le faire passer, puis refactorer.
5. **Build** debug + release, **zéro warning** (`-Wall -Wextra -Wpedantic`, warnings = erreurs en CI).
6. **Tests** : `ctest` complet + preset `asan` (AddressSanitizer + UBSan) sur la cible de tests.
7. **Vérification audio objective** (§15.6) pour tout ce qui touche au son.
8. **pluginval** niveau 5 (niveau 10 avant chaque merge) et `auval -v aumu Ddle Rwps`.
9. **Commit**, puis mise à jour de `PROGRESS.md`, `CHANGELOG.md`, et `DECISIONS.md` si un choix a été fait.

Si une étape échoue deux fois de suite pour la même raison : arrête-toi, résume le problème et les pistes dans `PROGRESS.md`, et demande à Gaby.

### 15.6 Vérifier le son sans pouvoir l'écouter
Tu ne peux pas écouter : construis des outils pour mesurer.
- **Cible CLI `DedaleRender`** : instancie le processor hors temps réel, charge un preset XML + une seed + un tempo, rend N secondes en WAV 48 kHz/24 bits dans `renders/` (ignoré par git).
- **Script `tools/analyse.py`** (numpy/scipy, lancé avec `python3 -I`) qui sort pour chaque WAV : peak, RMS, offset DC, crest factor, centroïde spectral, **mesure d'aliasing** (énergie au-dessus de la fondamentale non harmonique sur un sweep), détection de clics (dérivée seconde au-delà d'un seuil), détection de NaN/Inf, silence inattendu.
- **Tests de non-régression** : 10 rendus de référence (« golden ») versionnés en métriques JSON (pas les WAV), comparés avec tolérance. Un changement volontaire de son = mise à jour des golden **dans un commit séparé et justifié**.
- **Tests DSP unitaires** : VCO à la bonne fréquence (zéro-crossings ±0,5 %), 1V/oct juste sur 5 octaves (±3 cents), filtre : −3 dB au cutoff ±10 %, pas d'auto-oscillation à résonance max (sortie silencieuse sans entrée), VCA à 0 V = silence, enveloppe : temps de decay à −60 dB ±10 %.
- **Spectrogrammes PNG** générés pour chaque checkpoint d'écoute et joints à la PR.

### 15.7 Checkpoints avec Gaby
À la fin des phases 2, 3, 4, 5 et 7, produis :
- une PR comme décrit en 15.4 ;
- 3 à 6 rendus WAV courts avec leurs noms explicites (`seq_corrupt_0.wav`, `seq_corrupt_75.wav`, `tzfm_sweep.wav`, `fold_bias_sweep.wav`...) ;
- une liste de 5 à 10 questions d'écoute précises dans `LISTENING_CHECKLIST.md` (« Le folder à FOLD 70 % sonne-t-il trop dur ? Compare avec `fold_A.wav` et `fold_B.wav` »).
Ne passe pas à la phase suivante tant que Gaby n'a pas répondu. Note ses retours dans `CALIBRATION.md`.

### 15.8 Qualité, CI, release
- **CI GitHub Actions** (`macos-latest`) : configure, build release universal, tests, pluginval niveau 5, build du tutoriel web et tests Playwright (§16.5). Cache des dépendances FetchContent.
- `clang-format` appliqué avant chaque commit. `clang-tidy` en option dans un preset.
- Profiling CPU : benchmark automatique (`DedaleBench`) qui mesure le temps de `processBlock` (512 échantillons, ×4 oversampling) et échoue au-delà du budget fixé en phase 6.
- **Release v1.0** : version sémantique dans CMake, build universal, installeur `.pkg` (VST3 + AU + standalone) via `pkgbuild/productbuild`, et notes de version. Signature et notarisation Apple préparées dans un script mais **désactivées par défaut** (identifiants de Gaby nécessaires, ne jamais les écrire dans le repo).

---

## 16bis. DÉDALE fait partie d'une suite

DÉDALE est le **premier instrument d'une suite** de plugins Raw Powa Studio (nom de la suite : **LABRYS**). Ordre prévu :
1. **DÉDALE** : synthé génératif parallèle (ce document).
2. **TALOS** (provisoire) : synthé de percussion analogique, séquenceur 8 pas avec hauteur et vélocité par pas, 2 VCO avec FM et sync, bruit, filtre ladder 4 pôles LP/HP, decays très courts.
3. **ICARE** (provisoire) : synthé polyrythmique à sous-harmoniques (2 VCO, chacun avec 2 sous-oscillateurs diviseurs de fréquence, 4 générateurs rythmiques diviseurs d'horloge, 2 séquenceurs 4 pas).
4. **EUROPE** (provisoire) : synthé semi-modulaire **joué au clavier** (MIDI), 2 VCO, filtre ladder passe-bas + passe-haut, ADSR, LFO, arpégiateur/séquenceur, reverb à ressort, patch bay. Le plus « classique » des quatre, mais celui qui demande le filtre ladder et la reverb à ressort les plus soignés : il vient après ICARE.
5. **PHÈDRE** (provisoire) : petit synthé semi-modulaire mono, 1 VCO (dent de scie / impulsion à largeur variable), filtre ladder passe-bas 4 pôles, ADSR, LFO, sample & hold, petite patch bay, jouable au clavier MIDI. Le plus simple de la suite, il réutilise presque tout d'ARIANE (ladder de TALOS, ADSR et LFO d'EUROPE) : c'est le dernier.

Conséquence pour toi dès maintenant : **sépare dès la phase 0 ce qui est réutilisable** dans une bibliothèque partagée nommée **ARIANE** (`/libs/ariane`, cible CMake statique, sans dépendre de DÉDALE) :
- DSP : oscillateurs anti-aliasés, enveloppes, VCA, SVF, saturations, utilitaires d'oversampling et de lissage (le filtre ladder viendra avec TALOS et sera réutilisé par EUROPE : prévois son emplacement) ;
- séquençage : horloge interne/hôte sample-accurate, quantizer, RNG seedable ;
- patch bay : routeur par échantillon, jacks, normalisations ;
- UI : LookAndFeel, knobs, trimmers, LEDs, jacks, couche de câbles, thèmes ;
- outillage : `Render`, `analyse.py`, golden tests, bench, workflows CI, squelette du tuto web.

Le code propre à DÉDALE (séquenceur à bits, wavefolder, panneau) reste dans `/plugins/dedale`. Organise le repo en **monorepo** :
```
/libs/ariane
/plugins/dedale   (/plugins/talos, /plugins/icare, /plugins/europe, /plugins/phedre plus tard)
/tutorial         (un site, une section par instrument)
/docs
```
Ne sur-généralise pas : n'extrais dans ARIANE que ce qui est clairement générique, et note dans `DECISIONS.md` chaque élément extrait.

### Noms (univers du mythe crétois, à valider par Gaby)
| Nom | Rôle | Pourquoi |
|---|---|---|
| **LABRYS** | nom de la suite (5 synthés) | la double hache crétoise, d'où viendrait le mot « labyrinthe » |
| **ARIANE** | bibliothèque partagée | le fil qui relie tous les instruments |
| **DÉDALE** | synthé génératif | l'architecte du labyrinthe |
| **TALOS** | synthé de percussion | le géant de bronze, du métal qui frappe |
| **ICARE** | synthé à sous-harmoniques | la chute, comme les sous-harmoniques qui descendent |
| **EUROPE** | synthé semi-modulaire au clavier | la mère de Minos, donc la grand-mère d'Ariane |
| **PHÈDRE** | petit semi-modulaire mono | la petite sœur d'Ariane |

Centralise ces noms (nom affiché, nom de fichier, codes plugin 4 caractères, bundle ID) dans **un seul fichier** `/libs/ariane/cmake/SuiteIdentity.cmake` pour pouvoir les renommer d'un coup. Codes plugin proposés : fabricant `Rwps` ; `Ddle` (DÉDALE), `Tals` (TALOS), `Icar` (ICARE), `Eurp` (EUROPE), `Phed` (PHÈDRE). Identité visuelle commune (thèmes, typo, style des knobs et des câbles) dans ARIANE, avec une **couleur d'accent par instrument**.

### TORO reste en dehors
TORO (synthé de basse déjà existant, utilisé dans des projets Ableton de Gaby) **reste un projet séparé, dans son propre repo, et tu n'y touches pas**. Ne le déplace pas, ne le modifie pas, ne réutilise pas ses codes plugin. LABRYS démarre dans un **nouveau repo vierge**. Si on veut un jour faire profiter TORO d'ARIANE, ce sera un projet à part, décidé plus tard par Gaby.

---

## 16. Tutoriel web interactif

Un site statique dans `/tutorial` qui apprend à jouer de DÉDALE en manipulant des démos qui fonctionnent dans le navigateur. Public : quelqu'un qui connaît un peu la synthèse mais jamais vu de séquenceur à registre à décalage.

### 16.1 Stack
- **Vite + TypeScript**, sans framework lourd (vanilla ou Preact si besoin). **Web Audio API + AudioWorklet** pour le son, **Canvas/SVG** pour les visuels.
- Déploiement **GitHub Pages** via la CI (workflow séparé), site 100 % statique, aucun tracking.
- Responsive : doit être utilisable **sur téléphone** (contrôles tactiles, knobs manipulables au glisser vertical, cibles ≥ 44 px).
- Accessibilité : navigation clavier, `aria-label` sur chaque contrôle, contraste AA, option « réduire les animations ».
- Le son ne démarre qu'après un geste de l'utilisateur (bouton « Activer le son »), avec un volume général et un bouton panic.

### 16.2 Cohérence avec le plugin
- **Port TypeScript du séquenceur** (`tutorial/src/engine/sequencer.ts`) qui suit exactement §8.
- Pour garantir qu'il se comporte comme le C++ : la suite de tests C++ exporte des **fixtures JSON** (seed, actions, état attendu après N ticks) dans `tests/fixtures/`, et une suite **Vitest** rejoue ces mêmes fixtures sur la version TS. Une divergence fait échouer la CI.
- Le même RNG (même algorithme, même seed → même suite) des deux côtés.
- Une **mini-voix web** en AudioWorklet : sinus + triangle + TZFM, folder (même formule que `Calibration.h`, constantes exportées en JSON au build), SVF, decay. Version simplifiée assumée, mais le caractère doit être reconnaissable.
- Même identité visuelle que le plugin (thèmes, couleurs, typo, style de knobs et de LEDs), réimplémentée en CSS/SVG.

### 16.3 Chapitres (une page ou une section chacun)
Chaque chapitre suit le même schéma : **l'idée en 3 à 5 phrases → un widget interactif → un défi « Essaie ça » → « Dans le plugin » (quels contrôles, où ils sont sur le panneau)**.
1. **Vue d'ensemble** : schéma SVG cliquable du flux de signal (§2). Clic sur un bloc = description + lien vers son chapitre. Le chemin du signal s'anime selon ORDER.
2. **Les bits** : une rangée de 8 LEDs, boutons BIT FLIP / ADVANCE / RUN, LED play head et write head. On entend un clic par trigger et une note par pas actif. Affichage de la tension stockée sous chaque bit.
3. **Muter une séquence** : BIT SHIFT, LENGTH, knob CORRUPT avec un **compteur visible** des mutations (notes changées, bits flippés) et un historique défilant des 16 derniers cycles (grille façon piano-roll) pour *voir* la séquence évoluer. Bouton BUFFER save/recall.
4. **Deux séquences** : SEQ1 + SEQ2 avec longueurs différentes (polymétrie visualisée par deux cercles qui tournent), CHAIN SEQ, EG TRIG MIX.
5. **Le quantizer** : clavier visuel qui allume les notes de la gamme active, les 16 modes, CV RANGE, transposition par clic sur le clavier (simule le Note On MIDI).
6. **Les sources et la TZFM** : oscilloscope + spectre en temps réel, FM AMT, explication de ce que le « through-zero » change (comparaison A/B avec une FM linéaire classique).
7. **Le wavefolder** : courbe de transfert dessinée en direct (entrée → sortie), FOLD et BIAS qui la déforment, oscillo avant/après, spectre qui montre les harmoniques paires/impaires.
8. **Le filtre** : courbe de réponse en fréquence animée, morph LP→BP, résonance, modulation par EG1.
9. **Enveloppes et VCA** : tracé des decays, deux VCA, astuce « EG1 sur un VCA » pour des decays différents par chemin.
10. **ORDER et BLEND** : les trois configurations côte à côte, crossfade audible.
11. **La patch bay** : panneau de jacks simplifié où l'on tire des câbles (souris et tactile), normalisations affichées en pointillés qui disparaissent quand on patche. Trois patchs guidés : `M VCO → BIT FLIP 1`, `M VCO → CLOCK 2` (polymétrie), `EG1 → VCF VCA CV`.
12. **Recettes** : 6 à 8 patchs pas à pas orientés usage (sub dub, rimshot indus, cloche métallique, nappe de bruit, arpège qui se décompose, groove polymétrique). Chaque recette se charge en un clic dans la démo web **et** existe comme preset du plugin avec le même nom.
13. **Antisèche** : toutes les combinaisons de boutons (§8.4) en tableau filtrable, version imprimable.

### 16.4 Rédaction
- Textes en **français**, ton direct et concret, tutoiement, phrases courtes. Glossaire en fin de site (bit, registre à décalage, write head, normalisation, TZFM, wavefolder...).
- **Textes 100 % originaux** : n'emprunte aucune formulation, image ou schéma au manuel ou au site de Moog. Le nom « Labyrinth » n'apparaît pas.

### 16.5 Tests et critères d'acceptation du tuto
- Vitest : moteur séquenceur (fixtures partagées), quantizer, RNG.
- **Playwright** : chaque page se charge sans erreur console, chaque widget répond au clic/glisser, le son s'active, rendu mobile (viewport iPhone) sans débordement horizontal.
- Lighthouse : performance ≥ 90, accessibilité ≥ 95.
- Le site complet pèse moins de 2 Mo (hors polices), fonctionne hors ligne une fois chargé.
- Checkpoint avec Gaby : lien de preview (GitHub Pages ou `pnpm preview`) + liste de ce qu'il faut tester sur téléphone.
