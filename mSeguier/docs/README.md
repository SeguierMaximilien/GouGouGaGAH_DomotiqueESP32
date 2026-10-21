# mSeguier : carte domotique ESP32-C6 (SAE3.ESE.01.02)

Projet KiCad organisé selon les consignes de routage : `docs/`, `fab/`, `lib/`, `pretty/`,
`mSeguier.kicad_pro`, `mSeguier.kicad_sch`, `mSeguier.kicad_pcb`. Ouvre `mSeguier.kicad_pro` avec KiCad 9 ou 10 (fichiers au format KiCad 9).

## Carte
- 170 × 90 mm (maxi autorisé), coins arrondis R5, cotes sur Dwgs.User.
- Trous M3 (`MountingHole:MountingHole_3.2mm_M3`) en haut à gauche et en bas à droite, centres à 5 mm des bords, verrouillés.
- Placement : écran et RC522 en haut ; nanoESP32-C6 en bas à gauche, USB-C au bord gauche ;
  BH1750 sans rien au-dessus ; BMP280 en haut à droite, loin du MQ (chauffe) et du relais ;
  pont R1/R2 sous le MQ ; relais, diode et MOSFET ensemble ; bornier J1 au bord droit, fils vers l'extérieur.

## Règles (tableau des consignes)
| Classe | Isolation | Piste | Via | Réseaux |
|---|---|---|---|---|
| Forte tension | 1 mm | 1,2 mm | 1,6 / 0,6 | RELAY_NO, RELAY_COM, RELAY_NC |
| Fort courant | 0,6 mm | 1,2 mm | 1,6 / 0,6 | +5V (alim MQ et relais), GND (retour de masse), RELAY_COIL |
| Minimum | 0,25 mm | 0,4 mm | 1 / 0,6 | (non utilisée) |
| Default | 0,25 mm | 0,5 mm | 1,1 / 0,6 | tous les autres signaux, +3V3 |

Tailles prédéfinies : pistes 0,4 / 0,5 / 1,2 mm ; vias 1,6/0,6 et 1,8/0,6.
Pistes uniquement horizontales, verticales ou à 45°.

## Plans de masse
- GND sur les deux faces, toute la carte, isolation 0,8 mm, connexion en frein thermique.
- Zone d'exclusion « Exclusion 230V » sur les contacts du relais et le bornier (pas de plan de masse).
- Exception : pastilles 2 et 30 de U1 et pastille 2 de U3 en connexion pleine, les rayons thermiques y
  étant bloqués par les pistes voisines (sinon erreur DRC « frein thermique incomplet »). Ces deux
  pastilles sont aussi reliées par une piste GND.

## Composants
Pas de condensateurs : C1, C2 et C3 ont été retirés à la demande (les modules ont déjà les leurs).

## Vérifications faites
- ERC : 0 erreur, 0 avertissement.
- DRC (KiCad 9.0.1 et 10.0.7, zones re-remplies, parité schéma) : 0 erreur, 0 avertissement, 0 non routé.

## Ce qu'il te reste à faire
- Vérifier que le nom du projet (`mSeguier`) est bien le tien.
- Ajouter les docs des composants dans `docs/`.
- Envoyer le zip du dossier `fab/` sur jlcpcb.com et mettre la capture `mSeguier_jlcpcb_ok.png` dans `docs/`.
- Le relais est commandé par GPIO10 (broche 20 de la nano) : ajouter `#define RELAY_CMD 10` dans `pinout.h`.
- Les contacts du relais suivent la classe « Forte tension » ; en vrai 230 V il faudrait encore plus d'écart et une fente.
