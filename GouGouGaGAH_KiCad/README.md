# GouGouGaGAH_KiCad : schéma du projet domotique

Sous-projet KiCad du montage ESP32-C6. Le schéma reprend exactement le brochage de
`GouGouGaGAH_DomotiqueESP32/include/pinout.h`.

## Ouvrir le projet

1. Ouvre `GouGouGaGAH_KiCad.kicad_pro` avec KiCad (version 8, 9 ou 10).
2. Ouvre le schéma : tout est déjà relié et chaque composant a son empreinte.
3. Le circuit imprimé (`GouGouGaGAH_KiCad.kicad_pcb`) est placé et routé (voir plus bas).
   Si tu modifies le schéma, mets le PCB à jour avec *Outils > Mettre à jour le PCB depuis le
   schéma* (touche F8).

Le dossier contient sa propre bibliothèque, déclarée dans les tables du projet :

| Fichier | Contenu |
|---|---|
| `GouGouGaGAH.kicad_sym` | Symboles de la carte nanoESP32-C6 et des 5 modules (plus la DevKitC-1 en réserve) |
| `../Kad_Lib/ESP32_C6_modules.pretty/` | Tes empreintes : carte et 5 modules |
| `GouGouGaGAH.pretty/` | Empreinte ESP32-C6-DevKitC-1 (en réserve, non utilisée) |
| `sym-lib-table`, `fp-lib-table` | Déclarent ces bibliothèques pour ce projet uniquement |

Les résistances et condensateurs utilisent les bibliothèques standard de KiCad.
Garde le dossier `Kad_Lib` à côté de `GouGouGaGAH_KiCad`, le chemin est relatif.

## Circuit imprimé

- Carte double face de 165 × 114 mm, aperçus dans `rendu_pcb.png` (dessus) et `rendu_pcb_dessous.png`.
- GND : aucune piste, tout passe par un plan de masse sur les deux faces (rayons thermiques
  pour faciliter la soudure, liaison pleine sur U2 broche 2 et Q1 broche 3).
- Haut : écran (gauche) et RC522 (droite). Milieu : nanoESP32-C6 avec l'USB-C au bord gauche,
  BH1750 et BMP280 à côté. Bas : capteur MQ, pont R1/R2/C1, condensateurs C2/C3.
  À droite : relais, MOSFET, diode et bornier au bord de la carte.
- Les modules s'enfichent sur des barrettes : des pistes passent sous les modules, c'est normal.
  Aucune piste sous l'antenne de la nanoESP32-C6 (zone interdite de l'empreinte).
- Largeurs : 0,3 mm pour les signaux, 0,8 mm pour +5V / +3V3 / bobine du relais,
  1,5 mm pour les contacts du relais. Isolement 0,2 mm (0,5 mm pour les contacts du relais).
- Contrôle KiCad (DRC) : 0 erreur, 0 avertissement, 0 liaison manquante, PCB conforme au schéma.
- Si tu modifies des pistes, refais le remplissage des plans avec la touche B.
- **Attention** : les contacts du relais ne sont pas isolés pour du 230 V (il faudrait
  plusieurs millimètres d'écart et une fente). Ne commute que de la basse tension avec cette carte.

## Liaisons

| Réseau | ESP32-C6 | Relié à |
|---|---|---|
| I2C_SDA | GPIO6 | BH1750 SDA, BMP280 SDA |
| I2C_SCL | GPIO7 | BH1750 SCL, BMP280 SCL |
| SPI_SCK | GPIO18 | RC522 SCK, écran SCK, tactile T_CLK |
| SPI_MOSI | GPIO19 | RC522 MOSI, écran SDI, tactile T_DIN |
| SPI_MISO | GPIO20 | RC522 MISO, écran SDO, tactile T_DO |
| TFT_CS | GPIO21 | écran CS |
| TFT_DC | GPIO22 | écran DC |
| TFT_RST | GPIO23 | écran RESET |
| TOUCH_CS | GPIO4 | tactile T_CS |
| RFID_CS | GPIO11 | RC522 SDA/SS |
| RFID_RST | GPIO0 | RC522 RST |
| MQ_AO | GPIO1 | point milieu du pont R1/R2 (+ C1) |
| MQ_AO_5V | - | sortie AO du capteur MQ, entrée du pont R1/R2 |
| RELAY_CMD | GPIO10 | grille du MOSFET Q1 |
| RELAY_COIL | - | drain de Q1, bobine A2 du relais, anode de D1 |
| RELAY_NO / COM / NC | - | contacts 14 / 11 / 12 du relais, vers le bornier J1 (1 / 2 / 3) |
| +5V | broches 5V | capteur MQ (VCC), bobine A1 du relais, cathode de D1 |
| +3V3 | broche 3V3 | BH1750, BMP280 (VCC + CSB), RC522, écran (VCC + LED) |
| GND | GND | tout le monde, plus BH1750 ADDR et BMP280 SDO |

Broches non utilisées (croix « non connecté ») : MQ DO, RC522 IRQ, écran T_IRQ, et les GPIO libres de la carte.

## Composants et empreintes

| Réf. | Composant | Empreinte |
|---|---|---|
| U1 | MuseLab nanoESP32-C6 (2 x 15 broches) | `ESP32_C6_modules:MuseLab_nanoESP32-C6` |
| U2 | BH1750 (GY-302) | `ESP32_C6_modules:BH1750_GY-302` |
| U3 | BMP280 (GY-BMP280) | `ESP32_C6_modules:BMP280_GY-BMP280-3.3` |
| U4 | Capteur gaz MQ | `ESP32_C6_modules:MQ_Gas_Sensor_Module_FlyingFish` |
| U5 | RC522 | `ESP32_C6_modules:RFID_RC522_Module` |
| U6 | Écran ILI9341 2.4" + XPT2046 (MSP2402) | `ESP32_C6_modules:TFT_2.4in_SPI_ILI9341_MSP2402` |
| R1 | 1 kΩ | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal` |
| R2 | 2 kΩ | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal` |
| C1 | 100 nF | `Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm` |
| C2 | 10 µF (sur +3V3) | `Capacitor_THT:CP_Radial_D5.0mm_P2.00mm` |
| C3 | 10 µF (sur +5V) | `Capacitor_THT:CP_Radial_D5.0mm_P2.00mm` |
| Q1 | MOSFET BS270 (symbole BS170, même brochage D-G-S) | `Package_TO_SOT_THT:TO-92_Inline` |
| K1 | Relais Finder 30.22.7.005 (2 inverseurs, bobine 5 V) | `Relay_THT:Relay_DPDT_Finder_30.22` |
| D1 | Diode de roue libre 1N4007 | `Diode_THT:D_DO-41_SOD81_P10.16mm_Horizontal` |
| J1 | Bornier à vis 3 points, pas de 5 mm | `TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-3_1x03_P5.00mm_Horizontal` |

L'ordre des broches des symboles est identique à celui de tes empreintes (vérifié broche par broche).
Sur l'empreinte de la nanoESP32-C6, les textes de sérigraphie des broches 7 à 15 et 22 à 30 sont
décalés par rapport aux pastilles : la numérotation reste juste, c'est seulement l'affichage.

## Points d'attention

- **Capteur MQ** : alimenté en 5 V, sa sortie AO peut monter à 5 V alors que l'ESP32 n'accepte
  que 3,3 V. Le pont R1 (1 kΩ) / R2 (2 kΩ) ramène 5 V à 3,33 V au maximum.
- **RC522** : 3,3 V uniquement, jamais 5 V.
- **Écran** : VCC et LED sont en 3,3 V. Si ton module a un cavalier J1 à côté du régulateur,
  ferme-le pour qu'il fonctionne correctement en 3,3 V.
- **TOUCH_CS sur GPIO4** : GPIO4 est une broche de configuration au démarrage (MTMS), mais elle
  ne sert qu'au choix de l'interface JTAG, réglage désactivé par défaut. Aucun problème ici.
- **Bibliothèque Espressif** : son symbole ESP32-C6-DevKitC-1 oublie la broche 32 (GND) qui
  existe pourtant sur l'empreinte. Le symbole du projet la corrige.
- **Relais** : piloté par GPIO10 (libre dans `pinout.h`, à y ajouter : `#define RELAY_CMD 10`).
  Le BS270 commence à conduire vers 2,5 V au maximum : en 3,3 V ça marche, mais c'est un peu juste.
- **À vérifier sur tes composants** : la diode (corps noir = 1N4007, verre orange = 1N4148,
  empreinte `Diode_THT:D_DO-35_SOD27_P10.16mm_Horizontal`), le pas du bornier (5 mm ou 3,5 mm)
  et l'ordre des pattes du BS270 (D-G-S, méplat face à soi) au multimètre.
