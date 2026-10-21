# Contexte : projet domotique ESP32-C6 (GouGouGaGAH_DomotiqueESP32)

## Projet
- Carte : **MuseLab nanoESP32-C6** (module ESP32-C6-WROOM-1, CH343, 2× USB-C, 2 rangées de 15 broches espacées de 25,4 mm). Attention : ce n'est **pas** l'ESP32-C6-DevKitC-1 d'Espressif.
- Modules : BH1750 (GY-302), BMP280 (GY-BMP280), lecteur RFID RC522, écran TFT SPI 2.4" ILI9341 tactile (MSP2402), capteur de gaz MQ (module Flying-Fish).
- Code sous PlatformIO, schéma et PCB sous KiCad.
- Empreintes KiCad des modules : `Kad_Lib/ESP32_C6_modules.pretty` (déjà faites).

## Composants ajoutés par le prof

| Composant | Référence | Empreinte KiCad |
|---|---|---|
| Résistance 2 kΩ | 2 kΩ, 1/4 W, traversante | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal` |
| Résistance 1 kΩ | 1 kΩ, 1/4 W, traversante | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal` |
| Diode | Référence inconnue : 1N4148 si corps en verre orange, 1N4007 si corps noir | 1N4148 : `Diode_THT:D_DO-35_SOD27_P10.16mm_Horizontal` / 1N4007 : `Diode_THT:D_DO-41_SOD81_P10.16mm_Horizontal` |
| Transistor | **BS270** (onsemi), MOSFET canal N, 60 V / 400 mA, TO-92 (« JCASO » = code de fabrication) | `Package_TO_SOT_THT:TO-92_Inline` |
| Relais | **Finder 30.22.7.005**, 2 contacts inverseurs (DPDT), bobine 5 V continu | `Relay_THT:Relay_DPDT_Finder_30.22` |
| Bornier à vis | 3 entrées, pas de 5 mm probable | `TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-3_1x03_P5.00mm_Horizontal` |

## À vérifier
- **Diode :** couleur du corps pour savoir si c'est une 1N4148 ou une 1N4007.
- **Bornier :** environ 10 mm entre les vis extrêmes = pas de 5 mm ; environ 7 mm = pas de 3,5 mm (empreinte `PT-1,5-3-3.5-H_1x03_P3.50mm`).
- **BS270 :** ordre des pattes probablement D-G-S (1-2-3, méplat face à soi), comme le BS170. Non confirmé par la datasheet, à contrôler au multimètre.

## Rôle supposé (non confirmé par le prof)
- **BS270 + relais + diode :** commande du relais par une sortie de l'ESP. La diode de roue libre se place en parallèle de la bobine (A1/A2), cathode côté +5 V. Seuil du BS270 : 2,5 V max, donc piloté en 3,3 V ça passe mais c'est un peu juste.
- **1 kΩ + 2 kΩ :** probablement le pont diviseur sur la sortie AO du MQ (alimenté en 5 V). Le 1 kΩ va côté AO et le 2 kΩ côté GND, ce qui donne 5 V × 2/3 = 3,33 V max sur l'entrée de l'ESP.
