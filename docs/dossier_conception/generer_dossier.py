#!/usr/bin/env python3
"""Génère le dossier de conception AssistDomo (un seul PDF).

Usage (depuis la racine du dépôt) :
    python3 docs/dossier_conception/generer_dossier.py

Entrées :
  - GouGouGaGAH_Application/        code Arduino (annexe A)
  - mSeguier/docs/mSeguier_schema.pdf  schéma structurel KiCad
  - docs/dossier_conception/captures/*.png|jpg  captures Home Assistant (ordre alphabétique)
  - docs/dossier_conception/home_assistant/*.yaml  config HA (annexe B)
Sortie :
  - docs/dossier_conception/Dossier_conception_AssistDomo_SEGUIER.pdf
Nécessite : python3, pypdf, node + playwright (chromium).
"""
import base64
import html
import json
import pathlib
import subprocess
import sys
import tempfile

from pypdf import PdfReader, PdfWriter, Transformation, PageObject

ICI = pathlib.Path(__file__).resolve().parent
RACINE = ICI.parent.parent
APP = RACINE / "GouGouGaGAH_Application"
SCHEMA = RACINE / "mSeguier" / "docs" / "mSeguier_schema.pdf"
CAPTURES = ICI / "captures"
HA_DIR = ICI / "home_assistant"
SORTIE = ICI / "Dossier_conception_AssistDomo_SEGUIER.pdf"

AUTEUR = "Maximilien SEGUIER"
DATE = "1er novembre 2026"

CSS = """
@page { margin: 16mm 15mm 16mm 15mm; }
* { box-sizing: border-box; }
body { font-family: 'DejaVu Sans', 'Liberation Sans', sans-serif; font-size: 10.5pt;
       color: #1d2433; line-height: 1.4; margin: 0; }
h1 { font-size: 19pt; color: #123a6b; border-bottom: 2.5px solid #123a6b; padding-bottom: 4px;
     margin: 0 0 10px 0; }
h2 { font-size: 13.5pt; color: #123a6b; margin: 16px 0 6px 0; }
h3 { font-size: 11pt; color: #1d2433; margin: 12px 0 4px 0; font-family: 'DejaVu Sans Mono', monospace;
     background: #e8eef6; padding: 3px 6px; border-left: 4px solid #123a6b; }
p { margin: 4px 0 8px 0; }
.saut { break-before: page; }
table { border-collapse: collapse; width: 100%; margin: 6px 0 10px 0; font-size: 9.5pt; }
th, td { border: 1px solid #b7c2d3; padding: 3px 6px; text-align: left; vertical-align: top; }
th { background: #e8eef6; }
code { font-family: 'DejaVu Sans Mono', monospace; font-size: 9pt; }
pre.code { font-family: 'DejaVu Sans Mono', monospace; font-size: 7.4pt; line-height: 1.28;
           white-space: pre-wrap; word-break: break-all; margin: 0 0 6px 0; padding: 5px 7px;
           background: #fafbfd; border: 1px solid #d5dce8; counter-reset: l; }
pre.code span.l { display: block; padding-left: 3.6em; text-indent: -3.6em; }
pre.code span.l::before { counter-increment: l; content: counter(l); display: inline-block;
           width: 2.8em; margin-right: 0.8em; text-align: right; color: #9aa6b8; text-indent: 0; }
pre.code .c { color: #2f7d32; }
.couv { height: 255mm; display: flex; flex-direction: column; justify-content: space-between; }
.couv .haut { color: #123a6b; font-size: 12pt; }
.couv .titre { text-align: center; }
.couv .titre .t1 { font-size: 30pt; font-weight: bold; color: #123a6b; margin-bottom: 6px; }
.couv .titre .t2 { font-size: 16pt; color: #33415c; }
.couv .titre .t3 { font-size: 12pt; color: #5b6880; margin-top: 18px; }
.couv .bas { font-size: 11pt; border-top: 2px solid #123a6b; padding-top: 8px; }
.sommaire td { border: none; border-bottom: 1px dotted #b7c2d3; font-size: 11pt; padding: 6px 4px; }
.cadre { border: 2px dashed #9aa6b8; border-radius: 6px; height: 105mm; display: flex;
         align-items: center; justify-content: center; color: #7a869a; text-align: center;
         margin: 6px 0 12px 0; font-size: 10.5pt; }
figure { margin: 6px 0 14px 0; text-align: center; }
figure img { max-width: 100%; max-height: 205mm; border: 1px solid #b7c2d3; }
figcaption { font-size: 9.5pt; color: #5b6880; margin-top: 4px; }
.note { font-size: 9pt; color: #5b6880; }
"""

# ----------------------------------------------------------------- schéma fonctionnel ----

def boite(x, y, w, h, titre, contenu, couleur, ext=False):
    bord = "#8a94a6" if ext else couleur
    fond = "#f1f3f6" if ext else couleur + "18"
    style_bord = "2px solid" if ext else "2.5px dashed"
    return f"""<foreignObject x="{x}" y="{y}" width="{w}" height="{h}">
<div xmlns="http://www.w3.org/1999/xhtml" style="width:{w}px;height:{h}px;border:{style_bord} {bord};
border-radius:8px;background:{fond};padding:6px 8px;font-size:12px;line-height:1.3;
font-family:'DejaVu Sans',sans-serif;color:#1d2433;overflow:hidden">
<div style="font-weight:bold;color:{bord};font-size:13px;margin-bottom:3px">{titre}</div>{contenu}</div>
</foreignObject>"""


def fleche(points, label="", lx=None, ly=None, double=False, couleur="#1d2433", tirets=False, ancre="middle"):
    pts = " ".join(f"{x},{y}" for x, y in points)
    d = ' stroke-dasharray="6,4"' if tirets else ""
    debut = f' marker-start="url(#m_{couleur[1:]})"' if double else ""
    s = (f'<polyline points="{pts}" fill="none" stroke="{couleur}" stroke-width="2"{d}'
         f' marker-end="url(#m_{couleur[1:]})"{debut}/>')
    if label:
        lignes = label.split("\n")
        s += f'<text x="{lx}" y="{ly}" font-size="11" text-anchor="{ancre}" fill="{couleur}" font-family="DejaVu Sans">'
        for i, t in enumerate(lignes):
            s += f'<tspan x="{lx}" dy="{0 if i == 0 else 13}">{html.escape(t)}</tspan>'
        s += "</text>"
    return s


def schema_fonctionnel_svg():
    B, V, O, R, M, C = "#1f5fa8", "#2e7d32", "#c26a00", "#b3261e", "#6a3fa0", "#00838f"
    couleurs = ["#1d2433", "#c26a00", "#8a94a6"]
    marq = "".join(
        f'<marker id="m_{c[1:]}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" '
        f'orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="{c}"/></marker>' for c in couleurs)
    e = []
    # Entités extérieures
    e.append(boite(10, 70, 135, 150, "Habitation",
                   "Grandeurs physiques :<br>• lumière<br>• température<br>• pression<br>• gaz polluant", "", True))
    e.append(boite(10, 320, 135, 130, "Habitant",
                   "• présente un badge<br>• touche l'écran<br>• lit l'écran et la LED Tempo", "", True))
    e.append(boite(10, 560, 135, 80, "Énergie", "Câble USB-C 5 V<br>(PC ou chargeur)", "", True))
    e.append(boite(880, 40, 230, 120, "Home Assistant",
                   "Raspberry Pi 3 (HA OS), broker MQTT Mosquitto : tableau de bord, historique, automatisations",
                   "", True))
    e.append(boite(880, 185, 230, 85, "Internet / box",
                   "API Tempo EDF (couleur du jour)<br>Accès distant au tableau de bord par VPN Tailscale", "", True))
    e.append(boite(880, 470, 230, 62, "Porte", "Gâche électrique (contact NO/COM/NC)", "", True))
    e.append(boite(880, 560, 230, 70, "Éclairage / VMC", "Gradateur ou variateur commandé en PWM", "", True))
    # Fonctions de la carte (= cadres pointillés du schéma structurel)
    e.append(boite(230, 55, 230, 105, "FP2 Mesurer l'ambiance",
                   "Lumière : BH1750 (U2)<br>Température, pression : BMP280 (U3)<br><i>cadre « Capteurs I2C »</i>", V))
    e.append(boite(230, 185, 230, 110, "FP3 Détecter le gaz",
                   "Capteur MQ (U4) alimenté en 5 V<br>Pont diviseur R1 1 kΩ / R2 2 kΩ<br><i>cadre « Capteur de gaz MQ »</i>", O))
    e.append(boite(230, 320, 230, 135, "FP4 Dialoguer avec l'habitant",
                   "Écran TFT 2,4\" ILI9341 + tactile XPT2046 (U6)<br>Lecteur RFID RC522 (U5)<br><i>cadre « Bus SPI »</i>", M))
    e.append(boite(230, 555, 230, 95, "FP1 Alimenter",
                   "+5V (broche 5V) et +3V3 (régulateur) de la nanoESP32-C6<br><i>cadre « Alimentation »</i>", R))
    e.append(boite(540, 55, 250, 400, "FP5 Traiter et communiquer",
                   "<b>nanoESP32-C6 (U1)</b><br><br>• lit les capteurs, gère l'écran et les badges<br>"
                   "• mémorise les badges autorisés (NVS)<br>• WiFi 2,4 GHz : MQTT vers Home Assistant, "
                   "requête HTTP Tempo<br>• LED RGB WS2812 (GPIO8) : couleur Tempo du jour et du lendemain, "
                   "clignote en permanence<br>• sortie PWM 0-100 % (GPIO2)<br><br>"
                   "<i>cadre « Microcontrôleur »</i>", B))
    e.append(boite(880, 300, 230, 130, "FP6 Commander la gâche",
                   "MOSFET Q1 BS270<br>Diode de roue libre D1<br>Relais K1 Finder 30.22 (5 V)<br>Bornier J1<br>"
                   "<i>cadre « Commande du relais »</i>", C))
    # Liaisons
    N = "#1d2433"
    e.append(fleche([(145, 120), (230, 120)], "lumière\nT, P", 185, 98))
    e.append(fleche([(145, 200), (230, 240)], "gaz", 180, 205))
    e.append(fleche([(145, 400), (230, 400)], "badge\ntoucher", 185, 378, double=True))
    e.append(fleche([(460, 108), (540, 108)], "I2C\nSDA 6, SCL 7", 500, 92))
    e.append(fleche([(460, 240), (540, 240)], "tension\n0 à 3,3 V\nGPIO1", 500, 216))
    e.append(fleche([(460, 388), (540, 388)], "SPI 18/19/20\nCS 21, 4, 11\nRST 23, 0", 500, 352, double=True))
    e.append(fleche([(790, 100), (880, 100)], "WiFi\nMQTT", 835, 84, double=True))
    e.append(fleche([(790, 225), (880, 225)], "HTTP\nTempo", 835, 209, double=True))
    e.append(fleche([(790, 365), (880, 365)], "RELAY_CMD\nGPIO10", 835, 349))
    e.append(fleche([(995, 430), (995, 470)], "contacts", 1003, 455, ancre="start"))
    e.append(fleche([(720, 455), (720, 595), (880, 595)], "PWM GPIO2", 770, 588))
    e.append(fleche([(995, 160), (995, 185)], "", double=True, couleur="#8a94a6"))
    e.append(fleche([(145, 600), (230, 600)], "5 V", 185, 593))
    # Distribution de l'énergie
    Ocol = "#c26a00"
    e.append(fleche([(345, 555), (345, 455)], "+3V3", 352, 510, couleur=Ocol, tirets=True, ancre="start"))
    e.append(fleche([(460, 640), (820, 640), (820, 410), (880, 410)], "+5V, +3V3", 560, 633, couleur=Ocol, tirets=True))
    e.append(fleche([(660, 640), (660, 455)], "", couleur=Ocol, tirets=True))
    e.append(f'<text x="826" y="440" font-size="11" fill="{Ocol}" font-family="DejaVu Sans">+5V</text>'
             f'<text x="826" y="453" font-size="11" fill="{Ocol}" font-family="DejaVu Sans">bobine</text>')
    e.append(f'<polyline points="230,640 200,640 200,170 230,170" fill="none" stroke="{Ocol}" stroke-width="2" '
             f'stroke-dasharray="6,4" marker-end="url(#m_{Ocol[1:]})"/>'
             f'<polyline points="200,300 230,300" fill="none" stroke="{Ocol}" stroke-width="2" '
             f'stroke-dasharray="6,4" marker-end="url(#m_{Ocol[1:]})"/>')
    e.append(f'<text x="194" y="260" font-size="11" fill="{Ocol}" font-family="DejaVu Sans" '
             f'transform="rotate(-90 194 260)" text-anchor="middle">+3V3 / +5V</text>')
    # Légende
    e.append('<g font-family="DejaVu Sans" font-size="11">'
             '<rect x="230" y="672" width="26" height="14" fill="#1f5fa818" stroke="#1f5fa8" stroke-width="2" stroke-dasharray="5,3"/>'
             '<text x="262" y="683">Fonction de la carte (même découpage que les cadres pointillés du schéma structurel)</text>'
             '<rect x="230" y="694" width="26" height="14" fill="#f1f3f6" stroke="#8a94a6" stroke-width="2"/>'
             '<text x="262" y="705">Élément extérieur à la carte</text>'
             '<line x1="740" y1="701" x2="772" y2="701" stroke="#1d2433" stroke-width="2" marker-end="url(#m_1d2433)"/>'
             '<text x="778" y="705">information</text>'
             '<line x1="880" y1="701" x2="912" y2="701" stroke="#c26a00" stroke-width="2" stroke-dasharray="6,4" marker-end="url(#m_c26a00)"/>'
             '<text x="918" y="705" fill="#c26a00">énergie</text></g>')
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1120 715" width="100%">'
            f'<defs>{marq}</defs>{"".join(e)}</svg>')


# ----------------------------------------------------------------------- utilitaires ----

def bloc_code(texte, commentaires=True):
    lignes = []
    en_bloc = False
    for brut in texte.rstrip("\n").split("\n"):
        ligne = html.escape(brut.replace("\t", "    "))
        if commentaires:
            s = brut.lstrip()
            if en_bloc or s.startswith("/*"):
                ligne = f'<span class="c">{ligne}</span>'
                en_bloc = "*/" not in s
            elif s.startswith("//") or s.startswith(";") or s.startswith("#") and not s.startswith("#include") \
                    and not s.startswith("#define") and not s.startswith("#if") and not s.startswith("#e") \
                    and not s.startswith("#w") and not s.startswith("#pragma"):
                ligne = f'<span class="c">{ligne}</span>'
            elif "//" in brut and '"' not in brut.split("//")[0][-1:]:
                i = brut.find("//")
                if brut[:i].count('"') % 2 == 0:
                    ligne = html.escape(brut[:i]) + f'<span class="c">{html.escape(brut[i:])}</span>'
        lignes.append(f'<span class="l">{ligne if ligne else " "}</span>')
    return '<pre class="code">' + "".join(lignes) + "</pre>"


def image_data(p):
    mime = "image/png" if p.suffix.lower() == ".png" else "image/jpeg"
    return f"data:{mime};base64," + base64.b64encode(p.read_bytes()).decode()


# ------------------------------------------------------------------------- contenus ----

def partie_debut():
    return f"""
<div class="couv">
  <div class="haut">IUT de Toulon, département GEII<br>SAE3.ESE.01-02 Concevoir-Vérifier</div>
  <div class="titre">
    <div class="t1">Dossier de conception</div>
    <div class="t2">Assistant domotique « Yuki »<br>ESP32-C6 et Home Assistant</div>
    <div class="t3">Schéma fonctionnel · Schéma structurel · Captures Home Assistant<br>
    Annexes : code Arduino, codes et configuration Home Assistant</div>
  </div>
  <div class="bas"><b>{AUTEUR}</b><br>Enseignant : M. GIBERT<br>Rendu le {DATE}</div>
</div>
<div class="saut"></div>
<h1>Sommaire</h1>
<table class="sommaire">
<tr><td>1. Schéma fonctionnel</td></tr>
<tr><td>2. Schéma structurel (KiCad)</td></tr>
<tr><td>3. Captures d'écran Home Assistant</td></tr>
<tr><td>Annexe A. Code Arduino du programme final</td></tr>
<tr><td>Annexe B. Codes et configuration Home Assistant</td></tr>
</table>
<h2>Présentation du système</h2>
<p>L'assistant domotique <b>Yuki</b> affiche des informations sur l'habitation et permet de la piloter.
Une carte à base de <b>nanoESP32-C6</b> mesure la lumière (BH1750), la température et la pression (BMP280)
et la présence de gaz (MQ). Elle lit les badges RFID (RC522) pour commander la gâche de la porte
par le relais K1, affiche les données sur un écran tactile ILI9341 et fournit une sortie PWM pour
la gradation d'un éclairage ou la vitesse d'une VMC. La LED RGB de la carte indique la couleur Tempo EDF du jour
et du lendemain et clignote pour attester que le programme tourne.</p>
<p>La carte communique en WiFi avec <b>Home Assistant</b> (Raspberry Pi 3) par le protocole MQTT :
les entités sont créées automatiquement par la découverte MQTT, avec historique, tableau de bord
et automatisations. Le tableau de bord est accessible sur le réseau local et à distance par le VPN Tailscale, sans ouverture de port sur la box.</p>
"""


def partie_fonctionnel():
    return f"""
<h1>1. Schéma fonctionnel</h1>
{schema_fonctionnel_svg()}
"""


def partie_fonctionnel_suite():
    return """
<h2>Description des fonctions</h2>
<table>
<tr><th style="width:24%">Fonction</th><th style="width:30%">Composants</th><th>Rôle</th></tr>
<tr><td>FP1 Alimenter</td><td>nanoESP32-C6 (U1) : broches 5V et 3V3</td>
<td>Fournit le +5V (MQ, bobine du relais) et le +3V3 (capteurs I2C, écran, RFID) à partir de l'USB-C.</td></tr>
<tr><td>FP2 Mesurer l'ambiance</td><td>BH1750 GY-302 (U2), BMP280 (U3)</td>
<td>Mesure l'éclairement (lx), la température (°C) et la pression (hPa). Bus I2C partagé, adresses 0x23 et 0x76.</td></tr>
<tr><td>FP3 Détecter le gaz</td><td>Module MQ (U4), R1 1 kΩ, R2 2 kΩ</td>
<td>Donne une tension image de la concentration de gaz. Le pont diviseur ramène la sortie 0-5 V à 0-3,33 V
pour l'entrée analogique GPIO1.</td></tr>
<tr><td>FP4 Dialoguer avec l'habitant</td><td>Écran TFT 2,4" ILI9341 + XPT2046 (U6), RC522 (U5)</td>
<td>Affiche les mesures et les menus, reçoit les appuis tactiles, lit l'identifiant des badges. Bus SPI partagé,
un chip select par module.</td></tr>
<tr><td>FP5 Traiter et communiquer</td><td>nanoESP32-C6 (U1)</td>
<td>Exécute le programme : contrôle d'accès, menus, mémoire des badges en NVS, MQTT vers Home Assistant,
lecture de la couleur Tempo, LED RGB, sortie PWM.</td></tr>
<tr><td>FP6 Commander la gâche</td><td>BS270 (Q1), D1, relais Finder 30.22.7.005 (K1), bornier J1</td>
<td>Le MOSFET commute la bobine 5 V du relais depuis GPIO10. D1 protège Q1 de la surtension à la coupure.
Les contacts NO/COM/NC sortent sur J1 vers la gâche.</td></tr>
</table>
<h2>Liaisons entre fonctions</h2>
<table>
<tr><th style="width:24%">Liaison</th><th style="width:22%">Nature</th><th>Signaux (GPIO de l'ESP32-C6)</th></tr>
<tr><td>FP2 → FP5</td><td>numérique, I2C</td><td>SDA = GPIO6, SCL = GPIO7</td></tr>
<tr><td>FP3 → FP5</td><td>analogique</td><td>MQ_AO divisé, 0 à 3,33 V sur GPIO1 (ADC 12 bits)</td></tr>
<tr><td>FP4 ↔ FP5</td><td>numérique, SPI</td><td>SCK = GPIO18, MOSI = GPIO19, MISO = GPIO20 ; TFT_CS = 21, TFT_DC = 22, TFT_RST = 23,
TOUCH_CS = 4, RFID_CS = 11, RFID_RST = 0</td></tr>
<tr><td>FP5 → FP6</td><td>logique TOR</td><td>RELAY_CMD = GPIO10, actif à l'état haut</td></tr>
<tr><td>FP5 → éclairage / VMC</td><td>PWM</td><td>GPIO2, rapport cyclique 0 à 100 %</td></tr>
<tr><td>FP5 → LED RGB</td><td>série WS2812</td><td>GPIO8 (LED intégrée à la carte)</td></tr>
<tr><td>FP5 ↔ Home Assistant</td><td>WiFi 2,4 GHz, MQTT</td><td>topics <code>gougougagah/…</code> (voir annexe B)</td></tr>
<tr><td>FP5 ↔ Internet</td><td>WiFi, HTTP</td><td>api-couleur-tempo.fr : couleur du jour et du lendemain</td></tr>
</table>
"""


def partie_structurel_texte():
    return """
<h1>2. Schéma structurel</h1>
<p>Le schéma structurel a été saisi sous KiCad 10 (projet <code>mSeguier/mSeguier.kicad_sch</code>).
Il reprend le découpage du schéma fonctionnel : chaque fonction est entourée d'un cadre en trait pointillé
(Alimentation, Microcontrôleur, Capteurs I2C, Capteur de gaz MQ, Bus SPI, Commande du relais).
Le schéma est reproduit à la page suivante (original au format A3, en vectoriel : il peut être agrandi).</p>
<h2>Nomenclature des repères</h2>
<table>
<tr><th style="width:9%">Repère</th><th style="width:38%">Désignation</th><th style="width:20%">Fonction</th><th>Remarque</th></tr>
<tr><td>U1</td><td>MuseLab nanoESP32-C6 (2 × 15 broches)</td><td>FP5, FP1</td><td>Microcontrôleur, WiFi, LED RGB WS2812</td></tr>
<tr><td>U2</td><td>Module GY-302 BH1750</td><td>FP2</td><td>ADDR à GND : adresse 0x23</td></tr>
<tr><td>U3</td><td>Module GY-BMP280</td><td>FP2</td><td>CSB à 3V3 (mode I2C), SDO à GND : adresse 0x76</td></tr>
<tr><td>U4</td><td>Module capteur de gaz MQ</td><td>FP3</td><td>Alimenté en 5 V, DO non utilisé</td></tr>
<tr><td>R1, R2</td><td>Résistances 1 kΩ et 2 kΩ</td><td>FP3</td><td>5 V × 2k / (1k + 2k) = 3,33 V max</td></tr>
<tr><td>U5</td><td>Module RFID RC522</td><td>FP4</td><td>Alimenté en 3,3 V uniquement</td></tr>
<tr><td>U6</td><td>Écran TFT 2,4" ILI9341 + tactile XPT2046 (MSP2402)</td><td>FP4</td><td>Rétroéclairage sur 3V3</td></tr>
<tr><td>Q1</td><td>MOSFET canal N BS270</td><td>FP6</td><td>Commande de la bobine depuis GPIO10</td></tr>
<tr><td>D1</td><td>Diode 1N4004</td><td>FP6</td><td>Roue libre, cathode côté +5V</td></tr>
<tr><td>K1</td><td>Relais Finder 30.22.7.005.0010, 2RT, bobine 5 V</td><td>FP6</td><td>1er inverseur utilisé</td></tr>
<tr><td>J1</td><td>Bornier à vis 3 points</td><td>FP6</td><td>1 = NO, 2 = COM, 3 = NC</td></tr>
</table>
<h2>Dimensionnement de la commande du relais</h2>
<p>La bobine 5 V du relais Finder 30.22 consomme environ 40 mA (résistance d'environ 125 Ω).
Une sortie de l'ESP32-C6 ne peut pas fournir ce courant sous 5 V : le MOSFET BS270 (V<sub>GS(th)</sub> inférieure à 2,5 V,
I<sub>D</sub> max 400 mA) est commandé directement en 3,3 V par GPIO10 et commute la bobine côté masse.
La diode D1 en parallèle sur la bobine écoule le courant de self à l'ouverture et protège Q1.</p>
"""


def partie_ha():
    imgs = sorted([p for p in CAPTURES.glob("*") if p.suffix.lower() in (".png", ".jpg", ".jpeg")]) \
        if CAPTURES.exists() else []
    s = "<h1>3. Captures d'écran Home Assistant</h1>\n"
    s += ("<p>Home Assistant OS est installé sur un Raspberry Pi 3 du réseau local. La carte s'y connecte en WiFi "
          "par le broker MQTT Mosquitto ; l'appareil de Yuki (nommé <b>GouGouGaGAH</b> dans Home Assistant) et ses entités apparaissent "
          "automatiquement (découverte MQTT).</p>\n")
    if imgs:
        for i, p in enumerate(imgs, 1):
            legende = p.stem.split("_", 1)[-1].replace("_", " ").replace("-", " ")
            s += (f'<figure><img src="{image_data(p)}"><figcaption>Capture {i} : '
                  f'{html.escape(legende)}</figcaption></figure>\n')
    else:
        for t in ("Capture 1 : tableau de bord (mesures, porte, bouton d'ouverture, curseur d'éclairage)",
                  "Capture 2 : courbes de tendance (historique des capteurs)",
                  "Capture 3 : automatisations"):
            s += f'<div class="cadre">{t}<br>(capture à insérer)</div>\n'
    return s


def partie_annexe_code():
    ordre = ["platformio.ini", "include/pinout.h", "include/secrets_exemple.h", "src/main.cpp"]
    fichiers = [APP / f for f in ordre]
    for d in ("include", "src"):
        for p in sorted((APP / d).glob("*")):
            if p.suffix in (".h", ".cpp") and p not in fichiers and p.name != "secrets.h":
                fichiers.append(p)
    s = "<h1>Annexe A. Code Arduino de Yuki</h1>\n"
    s += ("<p>Projet PlatformIO, framework Arduino (plateforme pioarduino), carte <code>esp32-c6-devkitc-1</code>. "
          "Code source complet du programme final. <b>GouGouGaGAH</b> est le nom interne de Yuki dans le code, les noms de dossiers, les topics MQTT et l'appareil Home Assistant. Dépôt GitHub "
          "<code>SeguierMaximilien/GouGouGaGAH_DomotiqueESP32</code>, dossier <code>GouGouGaGAH_Application/</code>. "
          "Le fichier <code>secrets.h</code> (mots de passe WiFi et MQTT) n'est pas publié : "
          "seul son modèle <code>secrets_exemple.h</code> figure ici.</p>\n")
    s += "<table><tr><th>Fichier</th><th>Rôle</th></tr>"
    roles = {
        "main.cpp": "Initialisation des bus et des modules, boucle principale",
        "module_ha": "WiFi, MQTT, découverte Home Assistant, ordres reçus",
        "module_acces": "Contrôle d'accès : badges autorisés en NVS, ouverture de la porte 3 s",
        "module_menu": "Menus de l'écran tactile (Capteurs, Porte, Badges, Éclairage, Infos)",
        "module_tft": "Pilote de l'écran ILI9341 et du tactile XPT2046",
        "module_tempo": "Couleur Tempo EDF (tâche FreeRTOS) et LED RGB clignotante",
        "module_pwm": "Sortie PWM 0-100 % sur GPIO2",
        "module_relais": "Commande du relais K1 (GPIO10)",
        "module_rfid": "Lecture des badges RC522",
        "module_bh1750": "Capteur de lumière",
        "module_bmp280": "Capteur de température et de pression",
        "module_mq": "Capteur de gaz (valeur brute ADC)",
        "pinout.h": "Affectation des broches",
        "platformio.ini": "Configuration du projet et bibliothèques",
        "secrets_exemple.h": "Modèle des identifiants WiFi / MQTT",
    }
    vus = set()
    for p in fichiers:
        cle = p.stem if p.stem.startswith("module_") else p.name
        if cle in vus or cle not in roles:
            continue
        vus.add(cle)
        s += f"<tr><td><code>{html.escape(cle)}</code></td><td>{roles[cle]}</td></tr>"
    s += "</table>\n"
    for p in fichiers:
        if not p.exists():
            continue
        rel = p.relative_to(APP).as_posix()
        s += f"<h3>{html.escape(rel)}</h3>\n" + bloc_code(p.read_text(encoding="utf-8")) + "\n"
    return s


ENTITES = [
    ("sensor.gougougagah_luminosite", "Luminosité (lx)", "gougougagah/etat → lux"),
    ("sensor.gougougagah_temperature", "Température (°C)", "gougougagah/etat → temperature"),
    ("sensor.gougougagah_pression", "Pression (hPa)", "gougougagah/etat → pression"),
    ("sensor.gougougagah_gaz_brut", "Gaz (valeur brute 0-4095)", "gougougagah/etat → gaz"),
    ("sensor.gougougagah_badges_autorises", "Nombre de badges autorisés", "gougougagah/etat → badges"),
    ("sensor.gougougagah_tempo_aujourd_hui", "Couleur Tempo du jour", "gougougagah/etat → tempo_jour"),
    ("sensor.gougougagah_tempo_demain", "Couleur Tempo du lendemain", "gougougagah/etat → tempo_demain"),
    ("sensor.gougougagah_dernier_badge", "UID du dernier badge présenté", "gougougagah/badge → uid"),
    ("sensor.gougougagah_dernier_acces", "Résultat (autorise, refuse, ajoute…)", "gougougagah/badge → resultat"),
    ("binary_sensor.gougougagah_porte", "Porte ouverte / fermée", "gougougagah/porte (ON / OFF)"),
    ("button.gougougagah_ouvrir_la_porte", "Bouton d'ouverture de la porte", "gougougagah/porte/ouvrir"),
    ("number.gougougagah_eclairage", "Curseur PWM 0-100 %", "gougougagah/pwm/set, état gougougagah/pwm"),
]


def partie_annexe_ha():
    s = "<h1>Annexe B. Codes et configuration Home Assistant</h1>\n"
    s += ("<p class=\"note\">Dans Home Assistant, Yuki apparaît sous son nom interne <b>GouGouGaGAH</b> "
          "(appareil, entités <code>*.gougougagah_*</code>, topics <code>gougougagah/…</code>).</p>\n")
    s += """
<h2>B.1 Installation et intégration MQTT</h2>
<table>
<tr><th style="width:30%">Élément</th><th>Configuration</th></tr>
<tr><td>Serveur</td><td>Raspberry Pi 3, Home Assistant OS 18.4, en WiFi 2,4 GHz, adresse fixe 192.168.1.110,
interface web <code>http://192.168.1.110:8123</code></td></tr>
<tr><td>Broker MQTT</td><td>Module complémentaire <b>Mosquitto broker</b> (configuration par défaut, port 1883)</td></tr>
<tr><td>Compte de la carte</td><td>Utilisateur Home Assistant <code>esp32</code> (Paramètres → Personnes),
utilisé par la carte pour se connecter au broker</td></tr>
<tr><td>Intégration</td><td><b>MQTT</b> (Paramètres → Appareils et services), découverte activée,
préfixe <code>homeassistant</code></td></tr>
<tr><td>Accès distant</td><td>VPN <b>Tailscale</b> installé comme application Home Assistant sur le Pi : aucun port ouvert sur la box ; le tableau de bord est joint depuis un smartphone (réseau mobile) à l'adresse Tailscale du Pi, port 8123. Testé et fonctionnel.</td></tr>
</table>
<h2>B.2 Messages MQTT échangés</h2>
<table>
<tr><th style="width:30%">Topic</th><th style="width:16%">Sens</th><th>Contenu</th></tr>
<tr><td><code>gougougagah/etat</code></td><td>ESP32 → HA</td><td>JSON des mesures toutes les 10 s, ex. :
<code>{"lux":215.8,"temperature":21.4,"pression":1012.6,"gaz":812,"badges":2,"tempo_jour":"BLEU","tempo_demain":"BLANC"}</code></td></tr>
<tr><td><code>gougougagah/porte</code></td><td>ESP32 → HA</td><td><code>ON</code> (ouverte) / <code>OFF</code></td></tr>
<tr><td><code>gougougagah/porte/ouvrir</code></td><td>HA → ESP32</td><td>ordre d'ouverture (relais collé 3 s)</td></tr>
<tr><td><code>gougougagah/pwm</code>, <code>…/pwm/set</code></td><td>les deux</td><td>rapport cyclique 0 à 100</td></tr>
<tr><td><code>gougougagah/badge</code></td><td>ESP32 → HA</td><td><code>{"uid":"A1B2C3D4","resultat":"autorise"}</code></td></tr>
<tr><td><code>gougougagah/ordre</code></td><td>HA → ESP32</td><td>ordre texte d'une automatisation (pergola, volets…), affiché au moniteur série et sur l'écran Infos</td></tr>
<tr><td><code>gougougagah/dispo</code></td><td>ESP32 → HA</td><td><code>online</code> / <code>offline</code> (testament MQTT)</td></tr>
<tr><td><code>homeassistant/status</code></td><td>HA → ESP32</td><td><code>online</code> au redémarrage de HA : la carte renvoie ses annonces</td></tr>
</table>
<h2>B.3 Entités créées par découverte MQTT</h2>
<p>À chaque connexion, la carte publie (message retenu) une annonce sur
<code>homeassistant/&lt;type&gt;/gougougagah_&lt;id&gt;/config</code>. Aucune ligne de YAML n'est donc nécessaire
pour créer les entités. Exemple d'annonce pour la température :</p>
"""
    exemple = {
        "uniq_id": "gougougagah_temperature", "name": "Temperature", "stat_t": "gougougagah/etat",
        "val_tpl": "{{ value_json.temperature }}", "stat_cla": "measurement", "dev_cla": "temperature",
        "unit_of_meas": "°C",
        "dev": {"ids": ["gougougagah"], "name": "GouGouGaGAH", "mf": "Witherwatch", "mdl": "ESP32-C6"},
        "avty_t": "gougougagah/dispo"}
    s += bloc_code(json.dumps(exemple, ensure_ascii=False, indent=2), commentaires=False)
    s += "<table><tr><th>Entité</th><th>Grandeur</th><th>Source MQTT</th></tr>"
    for e, g, src in ENTITES:
        s += f"<tr><td><code>{e}</code></td><td>{g}</td><td><code>{src}</code></td></tr>"
    s += "</table>\n"
    yamls = sorted(HA_DIR.glob("*.yaml")) + sorted(HA_DIR.glob("*.yml")) if HA_DIR.exists() else []
    if yamls:
        s += "<h2>B.4 Tableau de bord, automatisations et scripts (YAML)</h2>\n"
        for p in yamls:
            s += f"<h3>{html.escape(p.name)}</h3>\n" + bloc_code(p.read_text(encoding="utf-8")) + "\n"
    return s


# --------------------------------------------------------------------------- rendu ----

def html_doc(corps, paysage=False):
    taille = "@page { size: A4 landscape; margin: 10mm 12mm; }" if paysage else "@page { size: A4; }"
    return f"""<!DOCTYPE html><html lang="fr"><head><meta charset="utf-8"><style>{CSS}{taille}</style></head>
<body>{corps}</body></html>"""


JS = r"""
const { chromium } = require('playwright');
(async () => {
  const jobs = JSON.parse(process.argv[2]);
  const b = await chromium.launch();
  const p = await b.newPage();
  for (const [src, dst, land] of jobs) {
    await p.goto('file://' + src, { waitUntil: 'load' });
    await p.pdf({ path: dst, preferCSSPageSize: true, printBackground: true,
      });
  }
  await b.close();
})();
"""


def numeroter(w):
    """Pied de page « Dossier de conception … · n / N » sur toutes les pages sauf la couverture."""
    import io
    from reportlab.pdfgen import canvas
    from reportlab.pdfbase import pdfmetrics
    from reportlab.pdfbase.ttfonts import TTFont
    pdfmetrics.registerFont(TTFont("DejaVu", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    n = len(w.pages)
    for i, page in enumerate(w.pages):
        if i == 0:
            continue
        pw, ph = float(page.mediabox.width), float(page.mediabox.height)
        buf = io.BytesIO()
        c = canvas.Canvas(buf, pagesize=(pw, ph))
        c.setFont("DejaVu", 7.5)
        c.setFillColorRGB(0.48, 0.53, 0.6)
        c.drawString(40, 18, "Dossier de conception AssistDomo - SAE3.ESE.01-02 - " + AUTEUR)
        c.drawRightString(pw - 40, 18, f"{i + 1} / {n}")
        c.save()
        buf.seek(0)
        page.merge_page(PdfReader(buf).pages[0])


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="dossier_"))
    parties = [
        ("1_debut", partie_debut(), False),
        ("2_fonctionnel", partie_fonctionnel(), True),
        ("3_fonctionnel_suite", partie_fonctionnel_suite() + '<div class="saut"></div>' + partie_structurel_texte(), False),
        ("5_suite", partie_ha() + '<div class="saut"></div>' + partie_annexe_code()
         + '<div class="saut"></div>' + partie_annexe_ha(), False),
    ]
    jobs = []
    for nom, corps, pays in parties:
        src = tmp / f"{nom}.html"
        src.write_text(html_doc(corps, pays), encoding="utf-8")
        jobs.append([str(src), str(tmp / f"{nom}.pdf"), pays])
    (tmp / "rendu.js").write_text(JS)
    subprocess.run(["node", str(tmp / "rendu.js"), json.dumps(jobs)], check=True)

    w = PdfWriter()
    for nom, _, _ in parties:
        for page in PdfReader(str(tmp / f"{nom}.pdf")).pages:
            w.add_page(page)
        if nom == "3_fonctionnel_suite":
            # Schéma structurel KiCad (A3 paysage) ramené au format A4 paysage, en vectoriel
            src = PdfReader(str(SCHEMA)).pages[0]
            sw, sh = float(src.mediabox.width), float(src.mediabox.height)
            A4W, A4H = 841.89, 595.28
            k = min(A4W / sw, A4H / sh)
            page = PageObject.create_blank_page(width=A4W, height=A4H)
            page.merge_transformed_page(src, Transformation().scale(k).translate(
                (A4W - sw * k) / 2, (A4H - sh * k) / 2))
            w.add_page(page)
    numeroter(w)
    w.add_metadata({"/Title": "Dossier de conception AssistDomo", "/Author": AUTEUR})
    with open(SORTIE, "wb") as f:
        w.write(f)
    print("OK :", SORTIE, len(w.pages), "pages")


if __name__ == "__main__":
    sys.exit(main())
