# SOCLE

[English](README.md) · **Français**

Un synthé basse soustractif, sale et analogique dans l'âme, en plugin : le compagnon de
[MONOLITHE](https://github.com/damcoll/monolithe-vst).
Gratuit et open source (AGPLv3). VST3 pour Windows et macOS, AU pour macOS.

![Façade de SOCLE](docs/facade.png)

Deux oscillateurs et un sub dans un filtre ladder façon Moog, deux enveloppes, un LFO, des effets intégrés
et 16 presets d'usine bien crasseux. Tout est sur la façade : pas de menus, pas de matrice de modulation.
SOCLE partage le look de MONOLITHE et sa section MACHINE : les deux sonnent comme sortis de la même vieille machine.

## Les commandes

**01 OSCILLATEURS**
- **FORME 1 / FORME 2** – scie ou carré pour chaque oscillateur.
- **ACCORD 2** – accord de l'oscillateur 2, ±24 demi-tons. **DÉSACCORD** – désaccord fin de l'oscillateur 2, ±50 centièmes.
- **OSC 1, OSC 2, SUB, BRUIT** – niveaux des deux oscillateurs, du sub (carré, une octave en dessous) et du bruit blanc.

**02 FILTRE** – passe-bas ladder à transistors, 24 dB/octave
- **COUPURE** – fréquence de coupure, 30 Hz – 18 kHz. Le filtre suit le clavier à moitié.
- **RÉSONANCE** – auto-oscille en haut de sa course.
- **DRIVE** – pousse l'entrée du filtre : il sature au lieu d'écrêter.
- **ENV** – de combien l'enveloppe de filtre l'ouvre (jusqu'à 7 octaves).
- **VÉLOCITÉ** – de combien la vélocité ouvre le filtre et change le volume.

**03 JEU**
- **MODE** – **POLY** (8 voix), **MONO** (une voix, la dernière note jouée gagne, chaque note relance les enveloppes) ou
  **LEGATO** (une voix ; les notes jouées liées glissent sans nouvelle attaque).
- **GLIDE** – temps de glissement vers la nouvelle note, 0 – 2 s. En POLY, chaque note glisse depuis la dernière jouée.

**04 ENVELOPPES** – deux ADSR aux courbes analogiques : **FILTRE** (coupure) et **AMPLI** (volume).

**05 LFO** – un triangle commun à toutes les voix ; son voyant le suit.
- **VITESSE** – 0,05 – 20 Hz. **QUANTITÉ** – profondeur.
- **DESTINATION** – **HAUT.** (hauteur : vibrato, jusqu'à ±1 octave), **FILTRE** (±3 octaves, wobble) ou
  **LARG.** (largeur d'impulsion : ne s'entend que sur les carrés).

**06 EFFETS** – dans cet ordre, après les voix
- **SATURATION** – à lampe, chaude puis granuleuse. Elle rend le son plus dense, pas plus fort.
- **CHORUS** – chorus stéréo façon Juno. Les graves restent au centre.
- **DELAY** – écho ping-pong calé sur le tempo du logiciel ; les répétitions s'assombrissent et s'amincissent, comme sur bande.
  **RETOUR** règle le nombre de répétitions, **TEMPS** la valeur de note (de 1/16 à 1/2, croche pointée par défaut).

**MACHINE** – la même que celle de MONOLITHE
- **DÉRIVE** – chaque voix se désaccorde lentement, chaque note sort un peu différente (justesse, brillance),
  les oscillateurs tournent librement. 0 = parfaitement stable.
- **POUSSIÈRE** – lo-fi : moins de bits, fréquence d'échantillonnage plus basse, son plus sombre, souffle et craquements. 0 = propre.

**Sortie** – volume **MASTER**, avec une saturation douce qui évite d'écrêter. Le **VU-mètre** suit la sortie et
affiche la dernière note jouée.

**MIDI** – pitch bend ±2 demi-tons, la molette de modulation (CC1) ouvre le filtre.

Toutes les commandes sont automatisables. Boutons et faders : glisser ou molette de la souris, double-clic pour revenir
à la valeur par défaut. La fenêtre se redimensionne de 75 % à 200 % (coin en bas à droite).

## Les presets

L'écran du haut affiche le preset en cours ; ◀ ▶ passent d'un preset à l'autre, un clic sur l'écran ouvre la liste.
Une étoile (`ROUILLE *`) signale que le son a été modifié depuis le chargement du preset.

**SAUVER** enregistre ton son sous un nom, dans un fichier `.socle` lisible rangé dans `Documents/SOCLE/Presets`
(tu peux les copier, les partager). « Ouvrir le dossier des presets », en bas de la liste, ouvre ce dossier.

| # | Preset | Son |
|---|---|---|
| 01 | ROUILLE | Scie + carré désaccordés, drive moyen : la basse à tout faire |
| 02 | BÉTON ARMÉ | Carré + sub, filtre fermé, grosse saturation |
| 03 | CÂBLE NU | Acide : scie, forte résonance, enveloppe courte, mono avec glide |
| 04 | GOUDRON | Sombre et lent, legato, LFO lent sur le filtre |
| 05 | SOUS-SOL | Sub presque pur, bruit et beaucoup de poussière : dub lo-fi |
| 06 | FONDERIE | Drive et résonance à fond : grognement industriel |
| 07 | LIMAILLE | Modulation de largeur d'impulsion métallique, chorus |
| 08 | FER BLANC | Basse pincée fine, écho pointé |
| 09 | TURBINE | Wobble : LFO rapide sur le filtre |
| 10 | HANGAR | Polyphonique, chorus et delay : basse en accords |
| 11 | RIVET | Carré très court et percussif |
| 12 | CHARBON | Reese : deux scies très désaccordées, beaucoup de dérive |
| 13 | SILO | Oscillateur 2 une octave au-dessus : son creux |
| 14 | DÉCHARGE | Bruit et poussière au maximum : son cassé |
| 15 | PYLÔNE | Mono, glide long, sub qui bourdonne |
| 16 | BITUME | Attaque lente qui gonfle |

## Installation

Téléchargez la dernière version sur la page [**Releases**](../../releases).

**Windows** – dézippez et copiez `SOCLE.vst3` dans `C:\Program Files\Common Files\VST3\`
(créez le dossier s'il n'existe pas ; Windows demandera les droits administrateur).

**macOS** – dézippez et copiez :
- `SOCLE.vst3` dans `~/Library/Audio/Plug-Ins/VST3/`
- `SOCLE.component` (AU) dans `~/Library/Audio/Plug-Ins/Components/`

Le plugin n'est pas signé par Apple. Si macOS le bloque, ouvrez le Terminal et tapez :

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/SOCLE.vst3
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/SOCLE.component
```

> ⚠️ **La version macOS n'a pas encore été testée sur un vrai Mac.** Elle est compilée et vérifiée automatiquement
> (pluginval et auval d'Apple), mais personne ne l'a encore jouée dans un logiciel. Vos retours sont les bienvenus :
> ouvrez une [issue](../../issues).

Relancez ensuite la recherche de plugins dans votre logiciel. SOCLE apparaît comme instrument (catégorie *Synth*), à côté de MONOLITHE.

## Compiler soi-même

Il faut CMake 3.22+, et Visual Studio 2022 (Windows) ou Xcode (macOS).

```
git clone --recursive https://github.com/damcoll/socle-vst.git
cd socle-vst
cmake -B build
cmake --build build --config Release
```

Les plugins sont dans `build/Socle_artefacts/Release/`. Deux petits outils sont compilés aussi :
- `SocleRender` – des lignes de basse et chaque preset d'usine rendus en .wav, pour régler le son sans logiciel ;
- `SocleSnapshot` – une image PNG de la fenêtre du plugin.

Les presets d'usine sont les fichiers du dossier `presets/` ; ils sont intégrés au plugin.

## Licence et crédits

SOCLE est publié sous licence **GNU AGPLv3**, voir [LICENSE](LICENSE).

- Construit avec [JUCE](https://juce.com) (AGPLv3) et le SDK VST3 de Steinberg (MIT).
- Polices : [Big Shoulders](https://github.com/xotypeco/big_shoulders) et [IBM Plex Mono](https://github.com/IBM/plex),
  sous licence SIL Open Font License (voir `assets/fonts`).
- Plugins vérifiés avec [pluginval](https://github.com/Tracktion/pluginval) de Tracktion.
