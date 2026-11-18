# mSeguier : carte domotique ESP32-C6 (SAE3.ESE.01.02)

Projet KiCad organisé selon les consignes de routage : `docs/`, `fab/`, `lib/`, `pretty/`,
`mSeguier.kicad_pro`, `mSeguier.kicad_sch`, `mSeguier.kicad_pcb`. Ouvre `mSeguier.kicad_pro` avec KiCad 10 (schéma et PCB au format KiCad 10, empreintes de la bibliothèque KiCad 10).

## Carte
- 170 × 90 mm (maxi autorisé), coins arrondis R5, cotes sur Dwgs.User.
- Trous M3 (`MountingHole:MountingHole_3.2mm_M3`) en haut à gauche et en bas à droite, centres à 5 mm des bords, verrouillés.
- Placement : écran et RC522 en haut ; carte ESP32-C6 (DevKit fourni par l'IUT) en bas à gauche, ses deux ports USB (UART et USB) dépassant d'environ 1 mm du bord gauche, antenne vers l'intérieur au-dessus d'une zone sans cuivre ;
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
Pistes uniquement horizontales, verticales ou à 45°, sans angle droit (coins cassés à 45°) et sans changement de largeur en cours de piste.

## Plans de masse
- GND sur les deux faces, toute la carte, isolation 0,8 mm, connexion en frein thermique.
- Zone d'exclusion « Exclusion 230V » sur les contacts du relais et le bornier (pas de plan de masse).
- Exception : pastille 17 de U1, pastille 2 de U3 et pastille 2 de U6 en connexion pleine, les rayons thermiques y
  étant bloqués par les pistes voisines (sinon erreur DRC « frein thermique incomplet »). 

## Composants
Pas de condensateurs : C1, C2 et C3 ont été retirés à la demande (les modules ont déjà les leurs).

## Historique
- 2026-11-18 : la carte MuseLab nanoESP32-C6 est remplacée par l'ESP32-C6 DevKit fourni par l'IUT (`lib/ESP32-C6DevKitGEII.kicad_sym`, `pretty/ESP32-C6DevKitGEII.pretty`). Mêmes GPIO qu'avant (conformes à `pinout.h`). Pistes re-routées (Freerouting 2.5, pistes à 45°), contacts du relais inchangés.
- Correction de l'empreinte fournie : ses pastilles étaient numérotées 1-26 puis 29-32, alors que le symbole va de 1 à 30. Les pastilles 29, 30, 31, 32 sont renommées 27, 28, 29, 30 (vérifié avec les noms des broches sérigraphiés : 27 = GPIO16/TX, 28 = GPIO3, 29 = GPIO2, 30 = GND). Sans ça, GND du symbole tombait sur GPIO3.

## Vérifications faites
- ERC : 0 erreur, 0 avertissement.
- DRC (KiCad 10.0.8, zones re-remplies, parité schéma) : 0 erreur, 0 avertissement, 0 non routé.
- Empreinte de la DevKit : la partie du contour sérigraphié qui dépasse du bord (côté USB) est passée sur F.Fab, sinon avertissement « sérigraphie coupée par le bord ».

## Ce qu'il te reste à faire
- Vérifier que le nom du projet (`mSeguier`) est bien le tien.
- Ajouter les docs des composants dans `docs/`.
- Envoyer le zip du dossier `fab/` sur jlcpcb.com et mettre la capture `mSeguier_jlcpcb_ok.png` dans `docs/`.
- Le relais est commandé par GPIO10 (broche 11 de la DevKit) : ajouter `#define RELAY_CMD 10` dans `pinout.h`.
- Les contacts du relais suivent la classe « Forte tension » ; en vrai 230 V il faudrait encore plus d'écart et une fente.
