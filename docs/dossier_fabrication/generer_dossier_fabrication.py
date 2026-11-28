#!/usr/bin/env python3
"""Génère le dossier de fabrication AssistDomo « Yuki » (un seul PDF).

Usage (depuis la racine du dépôt) :
    python3 docs/dossier_fabrication/generer_dossier_fabrication.py

Entrées :
  - mSeguier/ (projet KiCad) : nomenclature lue dans mSeguier.kicad_sch, rendus 3D dans mSeguier/docs/
  - docs/dossier_fabrication/figures/ : SVG exportés par kicad-cli (schéma, couches cuivre)
  - docs/dossier_fabrication/logos/*.png (facultatif) : logos IUT / Université affichés en couverture
  - GouGouGaGAH_DomotiqueESP32/ : programme de test (annexe 1)
Sortie :
  - docs/dossier_fabrication/Dossier_fabrication_AssistDomo_SEGUIER.pdf
Nécessite : python3, pypdf, reportlab, pygments, node + playwright (chromium), pdftotext.

Mise en page conforme au document « Normes de présentation d'un rapport scientifique ou
d'un document technique » (BUT GEII) : couverture, sommaire paginé à partir de 1, titres sans
point final ni article, figures et tableaux numérotés avec source, équations numérotées à droite,
annexes en chiffres romains, 4e de couverture avec résumé et abstract.
"""
import base64
import html
import json
import pathlib
import re
import subprocess
import tempfile
import io

from pypdf import PdfReader, PdfWriter
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from pygments import highlight
from pygments.lexers import CppLexer, IniLexer
from pygments.formatters import HtmlFormatter

ICI = pathlib.Path(__file__).resolve().parent
RACINE = ICI.parent.parent
KICAD = RACINE / "mSeguier"
TEST = RACINE / "GouGouGaGAH_DomotiqueESP32"
FIG = ICI / "figures"
LOGOS = ICI / "logos"
SORTIE = ICI / "Dossier_fabrication_AssistDomo_SEGUIER.pdf"

AUTEUR = "Maximilien SEGUIER"
ENSEIGNANT = "M. GIBERT"
DATE = "Janvier 2027"
PIED = "Dossier de fabrication – Assistant domotique Yuki – M. SEGUIER"

# --------------------------------------------------------------------------- styles ----

CSS = """
@page { size: A4; margin: 20mm 19mm 22mm 19mm; }
@page paysage { size: A4 landscape; margin: 12mm 12mm 18mm 12mm; }
* { box-sizing: border-box; }
body { font-family: 'Carlito', 'Calibri', sans-serif; font-size: 11pt; line-height: 1.15;
       color: #1a1a1a; margin: 0; text-align: justify; hyphens: auto; }
h1, h2, h3 { text-align: left; color: #1f3864; break-after: avoid; page-break-after: avoid; }
h1 { font-size: 16pt; margin: 18px 0 8px 0; border-bottom: 1.5px solid #1f3864; padding-bottom: 3px; }
h1.nonum { }
h2 { font-size: 13pt; margin: 14px 0 6px 0; }
h3 { font-size: 11.5pt; margin: 10px 0 4px 0; color: #2e5597; }
p { margin: 0 0 7px 0; }
ul, ol { margin: 0 0 7px 0; padding-left: 22px; }
li { margin-bottom: 2px; }
.saut { break-before: page; }
.garder { break-inside: avoid; }
table { border-collapse: collapse; width: 100%; margin: 4px 0 2px 0; font-size: 10pt;
        text-align: left; line-height: 1.2; }
th, td { border: 1px solid #8ea0bf; padding: 3px 5px; vertical-align: top; }
th { background: #dde4f0; }
tr { break-inside: avoid; }
.legende { font-size: 9.5pt; color: #333; text-align: center; margin: 3px 0 12px 0; font-style: italic; }
figure { margin: 6px 0 0 0; text-align: center; break-inside: avoid; }
figure img { max-width: 100%; }
.eq { display: flex; align-items: center; margin: 6px 0 8px 0; }
.eq .f { flex: 1; text-align: center; font-family: 'DejaVu Serif', serif; font-size: 11pt; }
.eq .n { width: 60px; text-align: right; }
code, .mono { font-family: 'DejaVu Sans Mono', monospace; font-size: 9pt; }
.att { border-left: 4px solid #c00000; background: #fbeaea; padding: 5px 8px; margin: 6px 0 9px 0; }
.info { border-left: 4px solid #2e5597; background: #eef2f9; padding: 5px 8px; margin: 6px 0 9px 0; }
/* couverture */
.couv { height: 253mm; display: flex; flex-direction: column; justify-content: space-between; text-align: center; }
.couv .logos { display: flex; justify-content: space-between; align-items: center; min-height: 22mm; }
.couv .logos img { max-height: 22mm; max-width: 60mm; }
.couv .dep { font-size: 13pt; color: #1f3864; }
.couv .t1 { font-size: 30pt; font-weight: bold; color: #1f3864; margin-bottom: 8px; }
.couv .t2 { font-size: 15pt; color: #2e5597; }
.couv .t3 { font-size: 12pt; color: #333; margin-top: 16px; }
.couv .bas { font-size: 12pt; border-top: 2px solid #1f3864; padding-top: 10px; text-align: left; }
.couv img.visuel { max-width: 120mm; margin: 0 auto; }
/* sommaire */
.sommaire { width: 100%; border: none; font-size: 11.5pt; }
.sommaire td { border: none; padding: 4px 2px; }
.sommaire td.p { text-align: right; width: 30px; }
.sommaire tr.n1 td { font-weight: bold; padding-top: 9px; }
.sommaire tr.n2 td.t { padding-left: 22px; }
.sommaire td.t span { background: #fff; padding-right: 4px; }
.sommaire td.t { background-image: radial-gradient(circle, #888 0.6px, transparent 0.8px);
                 background-size: 5px 3px; background-repeat: repeat-x; background-position: 0 80%; background-clip: content-box; background-origin: content-box; }
table.nomen { table-layout: fixed; font-size: 8.5pt; }
table.nomen th:nth-child(1) { width: 11%; } table.nomen th:nth-child(2) { width: 5%; }
table.nomen th:nth-child(3) { width: 21%; } table.nomen th:nth-child(4) { width: 13%; }
table.nomen th:nth-child(5) { width: 17%; } table.nomen th:nth-child(6) { width: 15%; }
table.nomen th:nth-child(7) { width: 18%; }
table.appro { table-layout: fixed; }
table.appro th:nth-child(1) { width: 8%; } table.appro th:nth-child(2) { width: 11%; }
table.appro th:nth-child(3) { width: 17%; } table.appro th:nth-child(4) { width: 11%; }
table.appro th:nth-child(5) { width: 7%; } table.appro th:nth-child(6) { width: 8%; }
table.nomen td { word-wrap: break-word; overflow-wrap: anywhere; hyphens: none; }
ol.biblio { list-style: none; padding-left: 0; text-align: left; }
ol.biblio li { margin-bottom: 5px; }
/* fiche d'intervention */
.fiche { font-size: 9pt; font-family: 'Liberation Sans', Arial, sans-serif; }
.fiche th { text-align: center; background: #e7e7e7; }
.fiche td.c { text-align: center; white-space: nowrap; }
.fiche td.num { white-space: nowrap; width: 38px; }
.fiche tr.sec td { background: #f0f0f0; font-weight: bold; }
.fiche td.vide { width: 62px; }
.entete td { font-size: 9.5pt; height: 20px; }
/* code */
pre.code { font-family: 'DejaVu Sans Mono', monospace; font-size: 7.6pt; line-height: 1.25;
           white-space: pre-wrap; word-break: break-all; background: #f6f8fa;
           border: 1px solid #d0d7de; padding: 5px 7px; margin: 0 0 8px 0; text-align: left; }
h3.fichier { font-family: 'DejaVu Sans Mono', monospace; font-size: 9.5pt; color: #1f3864;
             background: #dde4f0; padding: 3px 6px; margin: 10px 0 3px 0; }
.pivot { position: relative; width: 172mm; height: 240mm; break-inside: avoid; }
.pivot-in { position: absolute; width: 240mm; height: 172mm; left: 0; top: 0;
  transform-origin: top left; transform: translate(0, 240mm) rotate(-90deg); }
.pivot-in figure img { max-height: 160mm; }
.paysage figure img { max-height: 160mm; }
.qcouv { height: 253mm; display: flex; flex-direction: column; justify-content: center; }
.qcouv h2 { margin-top: 18px; }
.motscles { font-style: italic; }
"""


# --------------------------------------------------------------------- utilitaires ----

def e(t):
    return html.escape(t, quote=False)


def typo(h):
    """Espaces insécables de la typographie française, hors balises <pre> et <style>."""
    morceaux = re.split(r"(<pre.*?</pre>|<style.*?</style>|<[^>]+>)", h, flags=re.S)
    out = []
    for m in morceaux:
        if m.startswith("<"):
            out.append(m)
            continue
        m = re.sub(r" ([:;?!»%])", " \\1", m)
        m = re.sub(r"«\s", "« ", m)
        out.append(m)
    return "".join(out)


def data_uri(p):
    p = pathlib.Path(p)
    mime = {"png": "image/png", "jpg": "image/jpeg", "jpeg": "image/jpeg", "svg": "image/svg+xml"}[p.suffix[1:].lower()]
    return f"data:{mime};base64," + base64.b64encode(p.read_bytes()).decode()


class Num:
    """Numérotation des figures, tableaux et équations."""
    def __init__(self):
        self.fig = 0
        self.tab = 0
        self.eq = {}

    def figure(self, src, titre, source="figure réalisée par nos soins", style=""):
        self.fig += 1
        return (f'<figure><img src="{data_uri(src)}" style="{style}"></figure>'
                f'<div class="legende">Figure {self.fig} : {titre}. Source : {source}.</div>')

    def tableau(self, contenu, titre, source="tableau réalisé par nos soins"):
        self.tab += 1
        return (f'<div class="garder">{contenu}</div>'
                f'<div class="legende">Tableau {self.tab} : {titre}. Source : {source}.</div>')

    def equation(self, chap, formule):
        self.eq[chap] = self.eq.get(chap, 0) + 1
        return (f'<div class="eq"><div class="n"></div><div class="f">{formule}</div>'
                f'<div class="n">({chap}, {self.eq[chap]})</div></div>')


N = Num()


def table(entetes, lignes, cls=""):
    h = f'<table class="{cls}"><tr>' + "".join(f"<th>{x}</th>" for x in entetes) + "</tr>"
    for l in lignes:
        h += "<tr>" + "".join(f"<td>{x}</td>" for x in l) + "</tr>"
    return h + "</table>"


# ---------------------------------------------------------------- nomenclature ----

def lire_nomenclature():
    """Repères, valeurs et empreintes lus dans le schéma KiCad (symboles placés)."""
    t = (KICAD / "mSeguier.kicad_sch").read_text(encoding="utf-8")
    t = t[t.index("\n\t(symbol\n") :] if "\n\t(symbol\n" in t else t
    comps = {}
    for bloc in re.split(r"\n\t\(symbol\n", t)[1:]:
        ref = re.search(r'\(property "Reference" "([^"]+)"', bloc)
        val = re.search(r'\(property "Value" "((?:[^"\\]|\\.)*)"', bloc)
        fp = re.search(r'\(property "Footprint" "([^"]*)"', bloc)
        if not ref or ref.group(1).startswith("#"):
            continue
        comps[ref.group(1)] = (val.group(1).replace('\\"', '"'), fp.group(1) if fp else "")
    return comps


# Désignation, fonction et remarque de montage par repère (complète les champs KiCad).
DETAILS = {
    "U1": ("Carte ESP32-C6 DevKit (fournie par l'IUT), 2 × 15 broches", "Microcontrôleur, WiFi, alimentation",
           "Ports USB côté bord gauche ; antenne au-dessus de la zone sans cuivre"),
    "U2": ("Module GY-302 capteur de lumière BH1750", "Mesure de l'éclairement (I2C 0x23)",
           "Rien au-dessus du capteur (lumière)"),
    "U3": ("Module GY-BMP280 température et pression", "Mesure T et P (I2C 0x76)", "Version 3,3 V, 6 broches"),
    "U4": ("Module capteur de gaz MQ (type FlyingFish)", "Détection de gaz (sortie AO 0-5 V)",
           "Alimenté en 5 V ; élément chauffant"),
    "U5": ("Module lecteur RFID RC522", "Lecture des badges (SPI)", "3,3 V uniquement"),
    "U6": ("Écran TFT 2,4\" ILI9341 + tactile XPT2046 (MSP2402)", "Affichage et saisie tactile (SPI)",
           "Rétroéclairage relié au +3V3"),
    "R1": ("Résistance couche métal 1 kΩ, 0,25 W, 1 %", "Pont diviseur du capteur MQ", "Axiale, pas de 10,16 mm"),
    "R2": ("Résistance couche métal 2 kΩ, 0,25 W, 1 %", "Pont diviseur du capteur MQ", "Axiale, pas de 10,16 mm"),
    "Q1": ("Transistor MOSFET canal N BS270, TO-92", "Commande de la bobine du relais",
           "Brochage 1 = D, 2 = G, 3 = S"),
    "D1": ("Diode de redressement 1N4007, DO-41", "Roue libre de la bobine du relais",
           "Bague (cathode) côté +5V"),
    "K1": ("Relais Finder 30.22.7.005.0010, 2 RT, bobine 5 V CC", "Commande de la gâche",
           "Contacts 2 A / 250 V AC ; 2e inverseur non utilisé"),
    "J1": ("Bornier à vis 3 points, pas de 5 mm (Phoenix MKDS 1,5/3)", "Raccordement de la gâche",
           "1 = NO, 2 = COM, 3 = NC ; ouverture des vis vers l'extérieur"),
}


# ------------------------------------------------------------------- contenu ----

def couverture():
    logos = sorted(LOGOS.glob("*.png")) + sorted(LOGOS.glob("*.jpg")) if LOGOS.exists() else []
    if logos:
        bandeau = "".join(f'<img src="{data_uri(p)}">' for p in logos)
    else:
        bandeau = ('<div class="dep" style="text-align:left">Université de Toulon<br>IUT de Toulon</div>'
                   '<div class="dep" style="text-align:right">Département GEII<br>BUT GEII 2e année</div>')
    return f"""
<div class="couv">
  <div class="logos">{bandeau}</div>
  <div>
    <div class="dep">SAÉ 3.ESE.01-02 Concevoir – Vérifier</div>
    <div class="t1" style="margin-top:22mm">Dossier de fabrication</div>
    <div class="t2">Carte de l'assistant domotique « Yuki » à ESP32-C6</div>
    <div class="t3">Fabrication, contrôle et maintenance de la carte mSeguier :<br>
      schéma structurel, nomenclature, routage, procédure et protocole de test, fiche d'intervention</div>
  </div>
  <img class="visuel" src="{data_uri(FIG / "rendu_dessus.png")}">
  <div class="bas">
    <b>Auteur :</b> {AUTEUR}<br>
    <b>Enseignant encadrant :</b> {ENSEIGNANT}<br>
    <b>Date de remise :</b> {DATE} (année universitaire 2026-2027)
  </div>
</div>"""


SECTIONS = [
    # (niveau, ancre, titre)
    (1, "intro", "Introduction"),
    (1, "s1", "1. Schéma structurel"),
    (1, "s2", "2. Nomenclature des composants"),
    (2, "s21", "2.1 Liste des composants"),
    (2, "s22", "2.2 Approvisionnement et coût"),
    (1, "s3", "3. Routage de la carte"),
    (2, "s31", "3.1 Caractéristiques de la carte"),
    (2, "s32", "3.2 Règles de routage"),
    (2, "s33", "3.3 Faces dessus et dessous"),
    (2, "s34", "3.4 Fichiers de fabrication"),
    (1, "s4", "4. Procédure et protocole de test de la carte"),
    (2, "s41", "4.1 Matériel et consignes de sécurité"),
    (2, "s42", "4.2 Points de mesure"),
    (2, "s43", "4.3 Contrôles hors tension"),
    (2, "s44", "4.4 Mise sous tension et programmation"),
    (2, "s45", "4.5 Tests fonctionnels avec le programme de test"),
    (2, "s46", "4.6 Diagnostic des défauts courants"),
    (1, "s5", "5. Fiche d'intervention"),
    (1, "concl", "Conclusion"),
    (1, "biblio", "Bibliographie"),
    (1, "annexes", "Table des annexes"),
    (2, "a1", "Annexe 1 : code du programme de test"),
]


def sommaire(pages):
    lignes = ""
    for niv, ancre, titre in SECTIONS:
        p = pages.get(ancre, "")
        lignes += (f'<tr class="n{niv}"><td class="t"><span>{e(titre)}</span></td>'
                   f'<td class="p">{p}</td></tr>')
    return f'<h1 class="nonum" id="sommaire">Sommaire</h1><table class="sommaire">{lignes}</table>'


def h(niv, ancre):
    titre = next(t for n, a, t in SECTIONS if a == ancre)
    return f'<h{niv} id="{ancre}">{e(titre)}</h{niv}>'


def introduction():
    return f"""
<div class="saut"></div>
{h(1, "intro")}
<p>L'assistant domotique « Yuki » affiche des informations sur l'habitation et permet de la piloter
depuis Home Assistant. La carte mSeguier, conçue sous KiCad 10, regroupe autour d'une carte ESP32-C6
DevKit un capteur de lumière BH1750, un capteur de température et de pression BMP280, un capteur de
gaz MQ, un lecteur de badges RC522, un écran tactile ILI9341 et la commande d'une gâche électrique par
le relais K1.</p>
<p>Le dossier de conception a décrit le fonctionnement du système. Le présent dossier rassemble les
éléments nécessaires à la fabrication de la carte et à son contrôle :</p>
<ul>
<li>le schéma structurel et la nomenclature des composants à approvisionner ;</li>
<li>le routage de la carte et la liste des fichiers de fabrication transmis au fabricant JLCPCB ;</li>
<li>la procédure et le protocole de test, destinés à l'opérateur chargé du contrôle et de la maintenance ;</li>
<li>la fiche d'intervention, à remplir pour chaque carte contrôlée ou dépannée.</li>
</ul>
<p>Le code du programme de test utilisé par la procédure figure en annexe (annexe 1).</p>
<div class="info"><b>Erratum du dossier de conception.</b> Deux points du dossier de conception remis en
novembre 2026 sont corrigés ici : la carte microcontrôleur est l'ESP32-C6 DevKit fournie par l'IUT, et non une
carte MuseLab nanoESP32-C6 ; la diode de roue libre D1 est une 1N4007, et non une 1N4004. Le schéma structurel
et la nomenclature du présent dossier font foi.</div>
"""


def partie_schema():
    return f"""
<div class="saut"></div>
{h(1, "s1")}
<p>Le schéma structurel a été saisi sous KiCad 10 (projet <span class="mono">mSeguier</span>, format A3).
Il reprend le découpage du schéma fonctionnel du dossier de conception : chaque fonction est entourée
d'un cadre en trait pointillé (capteurs I2C, bus SPI, commande du relais, capteur de gaz, microcontrôleur,
points de test, alimentation). Le contrôle des règles électriques (ERC) ne signale aucune erreur.</p>
<p>Le schéma est reproduit sur la page suivante (cf. Figure 1). Le brochage de la carte ESP32-C6 est
identique à celui du programme de test (fichier <span class="mono">pinout.h</span>, annexe 1).</p>
<div class="saut"></div>
<div class="pivot"><div class="pivot-in">
{N.figure(FIG / "sch" / "mSeguier.svg", "Schéma structurel de la carte mSeguier, découpé en fonctions (cadres en pointillés)", "schéma réalisé par nos soins sous KiCad 10", "width:100%")}
</div></div>
"""


def partie_nomenclature():
    comps = lire_nomenclature()
    ordre = ["U1", "U2", "U3", "U4", "U5", "U6", "R1", "R2", "Q1", "D1", "K1", "J1"]
    lignes = []
    for r in ordre:
        if r not in comps:
            continue
        val, fp = comps[r]
        des, fct, rem = DETAILS[r]
        lignes.append([f"<b>{r}</b>", "1", e(des), e(val), f'<span class="mono" style="font-size:7.5pt">{e(fp.split(":")[-1])}</span>', e(fct), e(rem)])
    tp = sorted(r for r in comps if r.startswith("TP"))
    noms = ", ".join(f"{r} {e(comps[r][0])}" for r in tp)
    lignes.append([f"<b>TP1 à TP{len(tp)}</b>", str(len(tp)), "Points de test : pastilles CMS Ø 2 mm, face dessus, sans perçage",
                   noms, '<span class="mono" style="font-size:7.5pt">TestPoint_Pad_D2.0mm</span>', "Mesures à l'oscilloscope et au multimètre", "Aucun composant à monter"])
    lignes.append(["<b>H1, H2</b>", "2", "Trous de fixation Ø 3,2 mm non métallisés", "M3",
                   '<span class="mono" style="font-size:7.5pt">MountingHole_3.2mm_M3</span>', "Fixation de la carte", "Vis M3 et entretoises à prévoir"])
    lignes.append(["–", "1", "Circuit imprimé 170 × 90 mm, 2 couches, FR-4 1,6 mm, cuivre 35 µm", "mSeguier",
                   "–", "Support des composants", "Fabriqué par JLCPCB (fichiers du dossier fab)"])
    tab = table(["Repère", "Qté", "Désignation", "Valeur / référence", "Empreinte KiCad", "Fonction", "Remarque de montage"],
                lignes, "nomen")
    return f"""
{h(1, "s2")}
{h(2, "s21")}
<p>La nomenclature (cf. Tableau 1) liste les composants du schéma, avec leur valeur, leur empreinte et les
précautions de montage. Les repères, les valeurs et les empreintes sont ceux du projet KiCad. La carte ne
comporte aucun condensateur : les modules (capteurs, écran, lecteur RFID et carte ESP32-C6) intègrent déjà
leurs condensateurs de découplage.</p>
<div style="font-size:9pt">
{N.tableau(tab, "Nomenclature des composants de la carte mSeguier")}
</div>
<p>Les modules sont montés sur la face dessus. Le relais, la diode, le transistor, les résistances et le
bornier sont des composants traversants, soudés à la main sur la face dessous.</p>
{partie_appro()}
"""

APPRO = [
    # repère, fournisseur, référence fournisseur, prix unitaire HT, conditionnement, source, remarque
    ("U1", "Mouser", "356-ESP32C6DKC1N8", "8,15 €", "1", "[9]", "Équivalent Espressif ESP32-C6-DevKitC-1 ; la carte GEII (2 × 15 broches) est fournie par l'IUT et n'a pas le même brochage"),
    ("U2", "Stock87", "5037 (GY-302)", "1,95 €<sup>*</sup>", "1", "[10]", "Module identique à l'empreinte"),
    ("U3", "Otronic", "OT2313-B26", "1,17 €", "1", "[11]", "Module BMP280 3,3 V ; brochage 6 points à vérifier à la réception"),
    ("U4", "Otronic", "OT2052-D59", "2,25 €", "1", "[12]", "Module MQ-9"),
    ("U5", "Gotronic", "34942", "7,42 €", "1", "[13]", "Module Joy-it RC522"),
    ("U6", "–", "–", "n. c.", "–", "–", "Module MSP2402 non trouvé chez un distributeur reconnu ; l'Adafruit 2478 (RS 124-5490, 27,79 € HT) n'a pas le même brochage"),
    ("R1", "Farnell", "9341102 (MF25 1K)", "0,051 €", "10", "[14]", ""),
    ("R2", "Farnell", "MF25 2K (même série)", "0,051 €<sup>**</sup>", "10", "[14]", "Prix de la série, non relevé pour la valeur 2 kΩ"),
    ("Q1", "Farnell", "1017689", "0,499 €", "5", "[15]", "Produit en fin de vie : RS ne le vend plus [16]"),
    ("D1", "Farnell", "4245044", "0,106 €", "5", "[17]", ""),
    ("K1", "–", "–", "n. c.", "–", "[18] [19]", "Référence arrêtée chez Farnell et RS : prix non disponible. Un remplaçant doit garder l'empreinte Finder 30.22"),
    ("J1", "Farnell", "3041530", "1,74 €", "1", "[20]", "Phoenix Contact 1715734, pas de 5,08 mm. L'empreinte est au pas de 5,00 mm (référence exacte 1715721, prix non relevé) : écart de 0,16 mm sur 3 points"),
    ("PCB", "JLCPCB", "Cotation du 28/11/2026", "2,28 $", "5", "Figure 2", "11,40 $ HT les 5 cartes (FR-4, 2 couches, 1,6 mm), livraison DHL 23,21 $ en plus"),
]


def partie_appro():
    lignes = [[f"<b>{r}</b>", f, e(ref), prix, cond, src, e(rem)] for r, f, ref, prix, cond, src, rem in APPRO]
    tab = table(["Repère", "Fournisseur", "Référence fournisseur", "Prix unitaire HT", "Qté mini", "Source", "Remarque"], lignes, "appro")
    return f"""
{h(2, "s22")}
<p>Le Tableau 2 indique, pour chaque composant, un fournisseur reconnu, sa référence de commande et le prix
unitaire hors taxes relevé sur son site le 28 novembre 2026. Les distributeurs de composants (Farnell, Mouser)
sont retenus en priorité ; les modules pour Arduino, absents de leurs catalogues, proviennent de revendeurs
spécialisés (Gotronic, Otronic). Le prix de la carte nue est celui de la cotation JLCPCB.</p>
<div style="font-size:8.5pt">
{N.tableau(tab, "Approvisionnement des composants : fournisseur, référence et prix unitaire HT relevés le 28/11/2026", "sites des fournisseurs cités [9] à [20]")}
</div>
<p style="font-size:9pt"><sup>*</sup> Prix affiché sans mention HT ou TTC. <sup>**</sup> Prix de la référence 1 kΩ de la même série.</p>
{N.figure(KICAD / "docs" / "mSeguier_jlcpcb_ok.png", "Cotation de la carte mSeguier sur le site de JLCPCB : carte 2 couches de 170 × 90 mm détectée, 5 exemplaires pour 11,40 $ hors livraison", "capture d'écran réalisée par nos soins sur jlcpcb.com le 28/11/2026", "width:100%")}
<p>Hors relais, écran et circuit imprimé, le coût des composants s'élève à 23,39 € HT pour une carte
(somme des prix unitaires). Les quantités minimales de vente (lots de 5 ou de 10) portent le montant
réellement commandé à 26,73 € HT. Deux composants posent un problème d'approvisionnement : le relais Finder
30.22.7.005.0010 n'est plus distribué par Farnell ni par RS, et le transistor BS270 est en fin de vie. Une
révision de la carte devra choisir des équivalents disponibles et vérifier leur empreinte.</p>
"""


def partie_routage():
    regles = table(["Classe d'équipotentielle", "Isolation", "Largeur de piste", "Via (Ø / perçage)", "Réseaux"], [
        ["Forte tension", "1 mm", "1,2 mm", "1,6 / 0,6 mm", "RELAY_NO, RELAY_COM, RELAY_NC"],
        ["Fort courant", "0,6 mm", "1,2 mm", "1,6 / 0,6 mm", "+5V (MQ et bobine du relais), retour GND, commande de bobine"],
        ["Default", "0,25 mm", "0,5 mm", "1,1 / 0,6 mm", "+3V3 et tous les signaux"],
    ])
    fichiers = table(["Fichier (dossier fab)", "Contenu"], [
        ['<span class="mono">mSeguier-F_Cu.gtl</span>', "Cuivre face dessus (F.Cu)"],
        ['<span class="mono">mSeguier-B_Cu.gbl</span>', "Cuivre face dessous (B.Cu)"],
        ['<span class="mono">mSeguier-F_Mask.gts</span>', "Masque de soudure dessus (F.Mask)"],
        ['<span class="mono">mSeguier-B_Mask.gbs</span>', "Masque de soudure dessous (B.Mask)"],
        ['<span class="mono">mSeguier-F_Silkscreen.gto</span>', "Sérigraphie dessus (F.SilkS)"],
        ['<span class="mono">mSeguier-B_Silkscreen.gbo</span>', "Sérigraphie dessous (B.SilkS)"],
        ['<span class="mono">mSeguier-Edge_Cuts.gm1</span>', "Contour de la carte (Edge.Cuts)"],
        ['<span class="mono">mSeguier-PTH.drl</span>', "Perçages métallisés, format Excellon"],
        ['<span class="mono">mSeguier-NPTH.drl</span>', "Perçages non métallisés (trous de fixation), format Excellon"],
        ['<span class="mono">mSeguier-job.gbrjob</span>', "Fichier de description du jeu de Gerber"],
    ])
    return f"""
{h(1, "s3")}
{h(2, "s31")}
<ul>
<li>Dimensions : 170 × 90 mm (nombres entiers, valeurs maximales autorisées), coins arrondis de rayon 5 mm,
cotes placées sur la couche Dwgs.User.</li>
<li>Deux couches de cuivre, plan de masse GND sur chaque face (isolation 0,8 mm, connexion en frein thermique).</li>
<li>Trous de fixation M3 (Ø 3,2 mm) dans deux coins opposés, centres à 5 mm des bords.</li>
<li>Zone d'exclusion du plan de masse autour des contacts du relais et du bornier J1 (tensions supérieures à 50 V possibles).</li>
<li>Sérigraphie : « IUT GEII TOULON » sur la face dessus, nom de l'auteur sur la face dessous, noms des
signaux sur le bornier et sur les points de test.</li>
</ul>
<p>Le contrôle des règles de conception (DRC), effectué avec re-remplissage des zones et vérification de la
parité avec le schéma, ne signale ni erreur, ni avertissement, ni connexion non routée.</p>
{h(2, "s32")}
<p>Les largeurs de pistes et les isolations suivent les classes d'équipotentielles imposées par les consignes de
routage [1] (cf. Tableau 3). Les pistes sont horizontales, verticales ou à 45°, sans angle droit ni changement
de largeur. Les liaisons de masse passent uniquement par les plans de masse.</p>
{N.tableau(regles, "Classes d'équipotentielles utilisées pour le routage", "d'après les consignes de routage SAE3.ESE.01.02 [1]")}
{h(2, "s33")}
<p>La Figure 3 montre la face dessus : écran et lecteur RFID en haut, carte ESP32-C6 en bas à gauche avec ses
ports USB au bord de la carte, bloc de points de test sous le capteur de lumière, relais et bornier au bord droit.
La Figure 4 montre la face dessous, vue par transparence depuis le dessus puis retournée (vue miroir).</p>
{N.figure(FIG / "pcb_dessus.svg", "Routage de la face dessus (cuivre F.Cu, sérigraphie et contour)", "figure réalisée par nos soins sous KiCad 10", "width:100%")}
{N.figure(FIG / "pcb_dessous.svg", "Routage de la face dessous (cuivre B.Cu, sérigraphie et contour, vue miroir)", "figure réalisée par nos soins sous KiCad 10", "width:100%")}
{N.figure(FIG / "rendu_dessus.png", "Rendu 3D de la carte nue, face dessus", "figure réalisée par nos soins sous KiCad 10", "width:80%")}
{h(2, "s34")}
<p>Les fichiers de fabrication ont été générés dans le dossier <span class="mono">fab</span> du projet selon la
méthode préconisée par le fabricant JLCPCB [2] (cf. Tableau 4). Le dossier compressé est déposé sur le site du
fabricant pour obtenir un devis ; la capture d'écran de la cotation acceptée est placée dans le dossier
<span class="mono">docs</span> sous le nom <span class="mono">mSeguier_jlcpcb_ok</span>.</p>
{N.tableau(fichiers, "Fichiers de fabrication transmis au fabricant")}
"""


def partie_procedure():
    pm = table(["Point", "Signal", "Emplacement", "Valeur attendue"], [
        ["TP1", "MQ (MQ_AO)", "Bloc « POINTS DE TEST »", "Entrée GPIO1 après le pont R1/R2, 0 à 3,33 V"],
        ["TP2", "SDA (I2C_SDA)", "Bloc « POINTS DE TEST »", "Données I2C, GPIO6, 0 / 3,3 V"],
        ["TP3", "SCL (I2C_SCL)", "Bloc « POINTS DE TEST »", "Horloge I2C, GPIO7, 0 / 3,3 V, 100 kHz"],
        ["TP4", "RELAIS (RELAY_CMD)", "Bloc « POINTS DE TEST »", "Grille de Q1, GPIO10 : 0 V repos, 3,3 V travail"],
        ["TP5", "MQ 5V (MQ_AO_5V)", "Bloc « POINTS DE TEST »", "Sortie AO du capteur MQ avant le pont, 0 à 5 V"],
        ["+5V", "Alimentation 5 V", "U4 broche 1 (VCC) ou U1 broche 16", "4,75 à 5,25 V"],
        ["+3V3", "Alimentation 3,3 V", "U2 broche 1 (VCC) ou U1 broche 2", "3,14 à 3,47 V"],
        ["GND", "Masse", "U2 broche 2, U4 broche 2 ou U1 broche 1", "Référence de toutes les mesures"],
        ["A1 / A2", "Bobine du relais", "Broches A1 (+5V) et A2 (drain de Q1) de K1", "125 Ω hors tension ; 5 V au travail"],
    ])
    diag = table(["Symptôme", "Cause probable", "Vérification et action"], [
        ["Aucun message sur le moniteur série, écran éteint", "Câble USB, port USB, +5V absent ou en court-circuit",
         "Tester un autre câble ; mesurer +5V (test 3.1) ; contrôler l'isolement +5V / GND hors tension (test 2.4)"],
        ["« [BH1750] ECHEC » et « [BMP280] ECHEC »", "Bus I2C coupé ou en court-circuit, +3V3 absent",
         "Niveau haut 3,3 V sur TP2 et TP3 ; continuité U1 broches 6 et 7 vers U2 et U3 ; +3V3 sur U2 broche 1"],
        ["Un seul capteur I2C en échec", "Soudure du module, broche d'adresse",
         "Reprendre les soudures ; ADDR de U2 et SDO de U3 reliés à GND (adresses 0x23 et 0x76)"],
        ["« [RC522] ECHEC »", "SPI, RFID_CS (GPIO11), RFID_RST (GPIO0) ou alimentation",
         "Continuité des broches 1, 2, 3, 4 et 7 de U5 ; 3,3 V sur U5 broche 8 (jamais 5 V)"],
        ["Écran blanc ou noir", "TFT_CS (GPIO21), TFT_DC (GPIO22), TFT_RST (GPIO23), SPI",
         "Continuité des broches 3, 4, 5, 6 et 7 de U6 ; rétroéclairage (U6 broche 8) à 3,3 V"],
        ["Tactile sans réaction", "TOUCH_CS (GPIO4) ou bus SPI", "Continuité U1 broche 4 – U6 broche 11 et broches 10, 12, 13 de U6"],
        ["Valeur MQ bloquée à 0", "MQ non alimenté, R1 coupée ou TP1 en court-circuit",
         "+5V sur U4 broche 1 ; tension TP5 ; continuité de R1"],
        ["Valeur MQ bloquée à 4095", "R2 coupée : TP1 égal à TP5 (jusqu'à 5 V sur GPIO1)",
         "Couper l'alimentation immédiatement (risque pour l'ESP32) ; contrôler et remplacer R2"],
        ["Relais inactif, TP4 à 3,3 V", "Q1, bobine, D1 ou +5V", "Tension A1-A2 (test 4.8) ; brochage de Q1 ; résistance de bobine (test 2.3)"],
        ["Relais inactif, TP4 à 0 V", "GPIO10 ou programme", "Continuité U1 broche 11 – TP4 – Q1 grille ; message « [Relais] ON » sur le moniteur série"],
        ["Échauffement de Q1 ou de D1 au travail", "D1 montée à l'envers (court-circuit du +5V par Q1)",
         "Couper l'alimentation ; contrôler l'orientation de D1 (bague côté +5V)"],
        ["Contacts du bornier sans effet", "Soudure de K1 ou J1, contacts usés", "Continuité COM-NO et COM-NC (tests 2.5 et 4.8)"],
    ])
    return f"""
<div class="saut"></div>
{h(1, "s4")}
<p>La procédure décrit l'ordre des opérations confiées à l'opérateur ; le protocole fixe, pour chaque opération,
le matériel, le point de mesure et les limites d'acceptation. Les tests sont numérotés comme sur la fiche
d'intervention (partie 5) : chaque résultat y est reporté avec la date et le nom de l'opérateur. Une carte est
déclarée conforme lorsque tous les tests sont « OK » ou dans les limites. Au premier test non conforme, la
procédure s'arrête et le défaut est recherché à l'aide du tableau de diagnostic (cf. 4.6).</p>
<p>Les tests 1 et 2 se font hors tension, les tests 3 et 4 sous tension avec le programme de test ; le test 5
remet en service le programme final de l'assistant. Pour une maintenance, seuls les tests concernant la fonction
défaillante sont refaits, puis les tests 3 et 5.</p>
{h(2, "s41")}
<ul>
<li>Multimètre numérique (tension continue, résistance, continuité, test de diode).</li>
<li>Oscilloscope 2 voies, sondes ×10.</li>
<li>Ordinateur avec VS Code et PlatformIO, câble USB-C de données.</li>
<li>Testeur USB (tension et courant) entre l'ordinateur et la carte.</li>
<li>Badge RFID 13,56 MHz (MIFARE), stylet pour l'écran tactile, lampe de poche, briquet (gaz, sans flamme).</li>
<li>Thermomètre de référence et valeur de la pression atmosphérique locale.</li>
</ul>
<div class="att"><b>Sécurité.</b> Les contacts du relais K1 et le bornier J1 peuvent commuter une gâche ou une charge
reliée au secteur 230 V. Avant toute intervention, débrancher le câble USB et déconnecter du bornier J1 tout
circuit extérieur. Les tests de ce dossier se font sans charge sur J1. Le capteur MQ chauffe en fonctionnement :
ne pas le toucher pendant les tests. Le lecteur RC522 se détruit sous 5 V : ne jamais l'alimenter autrement que
par le +3V3 de la carte.</div>
<div class="info"><b>Remarque.</b> Le capteur MQ demande un préchauffage d'au moins 3 minutes avant une mesure
stable (24 h pour un capteur neuf, selon le fabricant). Le test 4.5 est donc réalisé en dernier parmi les
mesures du capteur de gaz.</div>
{h(2, "s42")}
<p>Les points de test TP1 à TP5 sont regroupés sous le capteur de lumière U2 (cf. Tableau 5). Les alimentations
et la masse n'ont pas de point de test : leurs valeurs sont constantes et se mesurent directement sur les broches
des modules. La pince de masse de l'oscilloscope se raccorde sur une broche GND d'un module.</p>
{N.tableau(pm, "Points de mesure utilisés par la procédure de test")}
{h(2, "s43")}
<h3>Test 1 : aspect visuel</h3>
<ul>
<li>Carte nue : absence de rayure ou de coupure de piste, sérigraphie lisible, perçages conformes.</li>
<li>Carte montée : soudures brillantes et coniques, sans pont entre pastilles ; aucune pastille non soudée.</li>
<li>Orientation : bague de D1 côté +5V (repère K), méplat de Q1 conforme à la sérigraphie, broche 1 des modules
du côté du repère carré.</li>
<li>Rien ne recouvre le capteur de lumière U2 ; les vis du bornier J1 sont accessibles.</li>
</ul>
<h3>Test 2 : contrôles électriques hors tension</h3>
<p>Câble USB débranché, multimètre en ohmmètre ou en continuité. Les mesures d'isolement se lisent après
stabilisation (charge des condensateurs des modules).</p>
<ul>
<li><b>2.1 Diode D1</b> : test de diode, pointe rouge sur A2 (anode), noire sur A1 (cathode) : tension de seuil
de 0,4 à 0,8 V ; en sens inverse : « OL ».</li>
<li><b>2.2 Pont diviseur</b> : R1 entre TP5 et TP1 (0,99 à 1,01 kΩ), R2 entre TP1 et GND (1,98 à 2,02 kΩ).
Mesures faites capteur MQ en place, l'impédance de sa sortie AO pouvant réduire légèrement la valeur lue.</li>
<li><b>2.3 Bobine du relais</b> : résistance entre A1 et A2, pointe rouge sur A1 : 112 à 138 Ω (125 Ω ± 10 % [3]).</li>
<li><b>2.4 Isolement des alimentations</b> : résistance +5V / GND, +3V3 / GND et +5V / +3V3 supérieure à 1 kΩ
(absence de court-circuit).</li>
<li><b>2.5 Contacts au repos</b> : continuité COM (J1-2) – NC (J1-3) inférieure à 1 Ω ; COM – NO (J1-1) : « OL ».</li>
</ul>
{h(2, "s44")}
<h3>Test 3 : alimentations</h3>
<p>Brancher le câble USB-C sur le port « UART » de la carte ESP32-C6 à travers le testeur USB.</p>
<ul>
<li><b>3.1</b> +5V entre U4 broche 1 et GND : 4,75 à 5,25 V.</li>
<li><b>3.2</b> +3V3 entre U2 broche 1 et GND : 3,14 à 3,47 V.</li>
<li><b>3.3</b> Courant absorbé lu sur le testeur USB, relais au repos : inférieur à 500 mA (limite d'un port USB 2.0).</li>
</ul>
<h3>Programmation</h3>
<ol>
<li>Ouvrir le projet PlatformIO <span class="mono">GouGouGaGAH_DomotiqueESP32</span> (programme de test, annexe 1) dans VS Code.</li>
<li>Lancer « Upload » ; en cas d'échec, maintenir le bouton BOOT de la carte pendant le début du transfert.</li>
<li>Ouvrir le moniteur série à 115 200 bauds et appuyer sur RESET.</li>
<li><b>3.4</b> Le message « === Diagnostic GouGouGaGAH === » s'affiche, suivi d'un compte rendu par module.</li>
</ol>
{h(2, "s45")}
<p>Le programme de test affiche un menu tactile avec un bouton par fonction : Lumière, Temp, Gaz, Badge, Tactile,
Tout et Relais. La barre de titre de chaque écran ramène au menu. Le moniteur série affiche en parallèle, toutes
les 2 s, les valeurs de lumière, de température, de pression et de gaz.</p>
<h3>Test 4 : fonctions de la carte</h3>
<ul>
<li><b>4.1 Détection des modules</b> : le moniteur série affiche « [BH1750] OK », « [BMP280] OK »,
« [RC522] OK (version registre 0x…) » avec une version différente de 0x00 et de 0xFF, « [Relais] OK ».</li>
<li><b>4.2 Bus I2C</b> : oscilloscope sur TP3 (SCL) et TP2 (SDA). Niveau haut de 3,0 à 3,47 V, niveau bas
inférieur à 0,4 V, fréquence de SCL pendant les trames de 90 à 110 kHz.</li>
<li><b>4.3 Capteur de lumière</b> (écran « Lumiere ») : valeur affichée cohérente avec l'éclairage de la pièce
(supérieure à 10 lx) ; capteur masqué de la main : inférieure à 5 lx ; lampe de poche à 5 cm : supérieure à 1 000 lx.</li>
<li><b>4.4 Température et pression</b> (écran « Temp ») : écart avec le thermomètre de référence inférieur à 2 °C,
pression comprise entre 950 et 1 050 hPa et à moins de 3 hPa de la pression locale.</li>
<li><b>4.5 Capteur de gaz</b> (écran « Gaz », après préchauffage) : mesurer V<sub>TP5</sub> et V<sub>TP1</sub>.
Le rapport du pont diviseur (équation 4, 1) doit être respecté à ± 3 %, et V<sub>TP1</sub> ne doit pas dépasser
3,47 V. La valeur brute affichée (0 à 4 095) doit correspondre à V<sub>TP1</sub> selon l'équation (4, 2), à ± 10 %.
Gaz d'un briquet non allumé approché du capteur : la valeur brute augmente d'au moins 200 points puis redescend.</li>
</ul>
{N.equation(4, "V<sub>TP1</sub> = V<sub>TP5</sub> × R2 / (R1 + R2) = V<sub>TP5</sub> × 2 / 3")}
{N.equation(4, "N<sub>brut</sub> ≈ 4 095 × V<sub>TP1</sub> / 3,3 V")}
<ul>
<li><b>4.6 Lecteur RFID et écran</b> (écran « Badge ») : le menu s'affiche sans pixel manquant ; le badge approché
à moins de 3 cm est lu et son identifiant s'affiche à l'écran et sur le moniteur série (« [RC522] Carte : … »).</li>
<li><b>4.7 Tactile</b> (écran « Tactile ») : toucher les quatre coins au stylet ; un point rouge apparaît à
chaque appui et les valeurs brutes de deux coins opposés diffèrent d'au moins 1 000 sur chaque axe.</li>
<li><b>4.8 Relais</b> (écran « Relais ») :
  <ul>
  <li>repos (OFF) : TP4 inférieur à 0,4 V ; tension A1-A2 inférieure à 0,25 V ; COM-NC fermé, COM-NO ouvert ;</li>
  <li>travail (ON, claquement audible) : TP4 de 3,0 à 3,47 V ; tension A1-A2 de 4,5 à 5,25 V (seuil d'enclenchement
  de la bobine : 3,7 V [3]) ; COM-NO inférieur à 1 Ω, COM-NC ouvert ;</li>
  <li>dix commutations successives sans défaut.</li>
  </ul></li>
</ul>
<h3>Test 5 : remise en service</h3>
<ul>
<li><b>5.1</b> Téléverser le programme final (projet <span class="mono">GouGouGaGAH_Application</span>) après
avoir renseigné le fichier <span class="mono">secrets.h</span> (réseau WiFi) et l'adresse du broker MQTT.</li>
<li><b>5.2</b> La LED RGB de la carte clignote dans la couleur Tempo du jour.</li>
<li><b>5.3</b> Les capteurs remontent dans Home Assistant (appareil « GouGouGaGAH ») et le relais s'y commande.</li>
<li><b>5.4</b> Un badge autorisé déclenche le relais pendant 3 s.</li>
</ul>
{h(2, "s46")}
<p>Le Tableau 6 regroupe les défauts les plus probables, leur cause et la vérification à réaliser. Il sert de guide
de maintenance : après réparation, les tests de la fonction concernée puis les tests 3 et 5 sont refaits et
consignés sur la fiche d'intervention.</p>
<div style="font-size:9.5pt">{N.tableau(diag, "Guide de diagnostic des défauts courants")}</div>
<p>Limite connue : la grille de Q1 n'a pas de résistance de rappel à la masse. Pendant le démarrage de l'ESP32,
avant l'exécution de <span class="mono">relais_setup()</span>, GPIO10 est en haute impédance et le relais peut
coller brièvement. Ce comportement n'est pas un défaut de fabrication ; une résistance de 100 kΩ entre grille
et source le supprimerait dans une révision de la carte.</p>
"""


# ------------------------------------------------------------- fiche d'intervention ----

FICHE = [
    ("1", "Aspect visuel", None),
    ("1.1", "Carte nue : pistes, sérigraphie, perçages", ("", "OK / NOK", "")),
    ("1.2", "Soudures sans pont ni manque", ("", "OK / NOK", "")),
    ("1.3", "Orientation D1 (bague côté +5V), Q1, modules", ("", "OK / NOK", "")),
    ("2", "Contrôles hors tension", None),
    ("2.1", "Seuil de D1 (A2 → A1)", ("0,4 V", "", "0,8 V")),
    ("2.2", "R1 entre TP5 et TP1", ("0,99 kΩ", "", "1,01 kΩ")),
    ("2.2", "R2 entre TP1 et GND", ("1,98 kΩ", "", "2,02 kΩ")),
    ("2.3", "Bobine K1 entre A1 et A2", ("112 Ω", "", "138 Ω")),
    ("2.4", "Isolement +5V / GND", ("1 kΩ", "", "")),
    ("2.4", "Isolement +3V3 / GND", ("1 kΩ", "", "")),
    ("2.4", "Isolement +5V / +3V3", ("1 kΩ", "", "")),
    ("2.5", "Repos : COM-NC fermé, COM-NO ouvert", ("", "OK / NOK", "")),
    ("3", "Alimentations et programmation", None),
    ("3.1", "+5V entre U4 broche 1 et GND", ("4,75 V", "", "5,25 V")),
    ("3.2", "+3V3 entre U2 broche 1 et GND", ("3,14 V", "", "3,47 V")),
    ("3.3", "Courant absorbé (relais au repos)", ("", "", "500 mA")),
    ("3.4", "Programme de test téléversé, message de démarrage", ("", "OK / NOK", "")),
    ("4", "Fonctions", None),
    ("4.1", "BH1750, BMP280, RC522, relais : « OK » au démarrage", ("", "OK / NOK", "")),
    ("4.2", "TP3 (SCL) : niveau haut", ("3,0 V", "", "3,47 V")),
    ("4.2", "TP3 (SCL) : fréquence", ("90 kHz", "", "110 kHz")),
    ("4.2", "TP2 (SDA) : niveau bas", ("", "", "0,4 V")),
    ("4.3", "Lumière ambiante", ("10 lx", "", "")),
    ("4.3", "Lumière, capteur masqué", ("", "", "5 lx")),
    ("4.3", "Lumière, lampe à 5 cm", ("1 000 lx", "", "")),
    ("4.4", "Écart de température / référence", ("– 2 °C", "", "+ 2 °C")),
    ("4.4", "Pression", ("950 hPa", "", "1 050 hPa")),
    ("4.5", "V(TP5) après préchauffage", ("", "", "5,25 V")),
    ("4.5", "V(TP1) / V(TP5)", ("0,647", "", "0,687")),
    ("4.5", "V(TP1)", ("", "", "3,47 V")),
    ("4.5", "Hausse de la valeur brute (gaz de briquet)", ("200", "", "")),
    ("4.6", "Affichage du menu, lecture du badge", ("", "OK / NOK", "")),
    ("4.7", "Tactile : 4 coins détectés", ("", "OK / NOK", "")),
    ("4.8", "OFF : tension TP4", ("", "", "0,4 V")),
    ("4.8", "ON : tension TP4", ("3,0 V", "", "3,47 V")),
    ("4.8", "ON : tension bobine A1-A2", ("4,5 V", "", "5,25 V")),
    ("4.8", "ON : COM-NO fermé, COM-NC ouvert", ("", "OK / NOK", "")),
    ("4.8", "10 commutations sans défaut", ("", "OK / NOK", "")),
    ("5", "Remise en service (programme final)", None),
    ("5.1", "Programme final téléversé", ("", "OK / NOK", "")),
    ("5.2", "LED RGB : couleur Tempo, clignotement", ("", "OK / NOK", "")),
    ("5.3", "Capteurs et relais dans Home Assistant", ("", "OK / NOK", "")),
    ("5.4", "Badge autorisé : relais 3 s", ("", "OK / NOK", "")),
]


def partie_fiche():
    entete = table(["Carte", "N° de série", "Motif de l'intervention", "Programme de test (version)"],
                   [["mSeguier (Yuki)", "", "☐ contrôle de fabrication  ☐ maintenance  ☐ réparation", ""]], "entete")
    lignes = ""
    for num, des, val in FICHE:
        if val is None:
            lignes += f'<tr class="sec"><td class="num">{num}</td><td colspan="6">{e(des)}</td></tr>'
        else:
            mini, valeur, maxi = val
            lignes += (f'<tr><td class="num">{num}</td><td>{e(des)}</td><td class="c">{mini}</td>'
                       f'<td class="c vide">{valeur}</td><td class="c">{maxi}</td><td class="vide"></td><td class="vide"></td></tr>')
    fiche = (f'<table class="fiche"><tr><th rowspan="2">Test N°</th><th rowspan="2">Désignation</th>'
             f'<th colspan="3">Résultats</th><th rowspan="2">Date</th><th rowspan="2">Nom</th></tr>'
             f'<tr><th>Mini</th><th>Valeur</th><th>Maxi</th></tr>{lignes}</table>')
    defaut = table(["Défaut constaté", "Composant ou liaison en cause", "Action réalisée", "Test(s) refait(s)"],
                   [["&nbsp;<br>&nbsp;", "", "", ""], ["&nbsp;<br>&nbsp;", "", "", ""]], "fiche")
    maj = table(["Indice", "Date", "Objet de la modification"], [
        ["00", "01/2027", "Rédaction et diffusion du document (carte mSeguier, programme de test GouGouGaGAH_DomotiqueESP32)"],
        ["&nbsp;", "", ""], ["&nbsp;", "", ""]], "fiche")
    return f"""
<div class="saut"></div>
{h(1, "s5")}
<p>La fiche d'intervention reprend la numérotation des tests de la partie 4. Une fiche est remplie par carte et par
intervention ; elle est conservée avec la carte. La colonne « Valeur » reçoit la mesure ou la mention OK / NOK.
Le tableau de mise à jour trace les évolutions de la fiche, sur le modèle de l'exemple fourni [4].</p>
{entete}
<div style="height:6px"></div>
{fiche}
<p style="font-size:9pt;margin-top:6px"><b>Conclusion de l'intervention :</b> ☐ carte conforme &nbsp;&nbsp;
☐ carte non conforme &nbsp;&nbsp; Visa de l'opérateur : ………………………………</p>
<h3>Défauts constatés et réparations</h3>
{defaut}
<h3>Mise à jour de la fiche</h3>
{maj}
"""


def conclusion():
    return f"""
<div class="saut"></div>
{h(1, "concl")}
<p>Le dossier rassemble les éléments nécessaires à la fabrication et au contrôle de la carte mSeguier. Le projet
KiCad est cohérent : ERC et DRC sans erreur ni avertissement, schéma et circuit imprimé en parité, fichiers de
fabrication à jour dans le dossier <span class="mono">fab</span>. La nomenclature précise les douze composants à
monter, les cinq points de test et le circuit imprimé à commander.</p>
<p>La procédure de test vérifie chaque fonction de la carte avec un multimètre, un oscilloscope et le programme
de test : alimentations, bus I2C, capteurs, lecteur RFID, écran tactile et commande du relais. Les limites
d'acceptation chiffrées et le guide de diagnostic permettent à un opérateur de déclarer une carte conforme ou de
localiser un défaut, puis de consigner son intervention sur la fiche prévue à cet effet.</p>
<p>Une révision de la carte pourrait ajouter une résistance de rappel sur la grille de Q1 et un point de test sur
le bus SPI, aujourd'hui accessible uniquement sur les broches de l'écran et du lecteur RFID.</p>
<div class="saut"></div>
{h(1, "biblio")}
<ol class="biblio">
<li>[1] GIBERT. <i>SAE3.ESE.01.02 Concevoir-Vérifier, Assistant domotique : consignes de routage</i>.
IUT de Toulon, département GEII, 2026. Document interne (Moodle).</li>
<li>[2] JLCPCB. <i>How to generate Gerber and drill files in KiCad 9</i> [en ligne].
Disponible sur : https://jlcpcb.com/help/article/how-to-generate-gerber-and-drill-files-in-kicad-9</li>
<li>[3] FINDER. <i>Série 30, relais miniature pour circuit imprimé 2 A</i> [en ligne]. Fiche technique.
Disponible sur : https://www.farnell.com/datasheets/2626620.pdf</li>
<li>[4] IUT de Toulon, département GEII. <i>Exemple de fiche d'intervention</i>. Document interne (Moodle), 2026.</li>
<li>[5] ONSEMI. <i>BS270, N-Channel Enhancement Mode Field Effect Transistor</i> [en ligne]. Fiche technique.
Disponible sur : https://www.onsemi.com/download/data-sheet/pdf/bs270-d.pdf</li>
<li>[6] BOSCH SENSORTEC. <i>BMP280 Digital Pressure Sensor, datasheet BST-BMP280-DS001</i> [en ligne].
Disponible sur : https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf</li>
<li>[7] ESPRESSIF SYSTEMS. <i>ESP32-C6, Get Started (ESP-IDF v5.5.1)</i> [en ligne].
Disponible sur : https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32c6/get-started/index.html</li>
<li>[8] HANDSONTEC. <i>RC522 RFID Development Kit</i> [en ligne]. Disponible sur : https://www.handsontec.com/dataspecs/RC522.pdf</li>
<li>[9] MOUSER ELECTRONICS. <i>ESP32-C6-DevKitC-1-N8</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://eu.mouser.com/ (référence 356-ESP32C6DKC1N8)</li>
<li>[10] STOCK87. <i>Module GY-302 capteur de luminosité digital BH1750</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://www.stock87.fr/module-gy302-capteur-de-luminosite-digital-bh1750-xml-243_363-920.html</li>
<li>[11] OTRONIC. <i>Module de capteur de pression barométrique numérique BMP280</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://www.otronic.fr/fr/module-de-capteur-de-pression-barometrique-numerique-bmp280</li>
<li>[12] OTRONIC. <i>Module capteur de gaz MQ-9</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://www.otronic.fr/fr/module-capteur-de-gaz-mq-9</li>
<li>[13] GO TRONIC. <i>Module RFID 13,56 MHz TAG RC522</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://www.gotronic.fr/art-module-rfid-13-56-mhz-tag-rc522-25651.htm</li>
<li>[14] FARNELL. <i>MF25 1K, résistance traversante 1 kΩ 250 mW 1 %</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://fr.farnell.com/multicomp-pro/mf25-1k/resistance-1k-0-25w-1/dp/9341102</li>
<li>[15] FARNELL. <i>BS270, MOSFET canal N 60 V TO-92</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://fr.farnell.com/onsemi/bs270/transistor-mosfet-n-to-92/dp/1017689</li>
<li>[16] RS COMPONENTS. <i>BS270 onsemi</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://fr.rs-online.com/web/p/transistors-mosfet/8075184</li>
<li>[17] FARNELL. <i>1N4007 Multicomp Pro, diode 1 kV 1 A DO-41</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://fr.farnell.com/multicomp-pro/1n4007/diode-simple-1kv-1a-do-41/dp/4245044</li>
<li>[18] FARNELL. <i>Finder 30.22.7.005.0010</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://fr.farnell.com/finder/30-22-7-005-0010/relais-ci-dpco-5vdc/dp/1217368</li>
<li>[19] RS COMPONENTS. <i>Finder 30.22.7.005.0010</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://fr.rs-online.com/web/p/relais-de-signaux/4926580</li>
<li>[20] FARNELL. <i>Phoenix Contact 1715734, bornier 3 points au pas de 5,08 mm</i> [en ligne]. Consulté le 28/11/2026. Disponible sur : https://nl.farnell.com/en-NL/phoenix-contact/1715734/terminal-block-wire-to-brd-3pos/dp/3041530</li>
</ol>
"""


# --------------------------------------------------------------------- annexes ----

def bloc_code(chemin):
    texte = chemin.read_text(encoding="utf-8")
    lexer = IniLexer() if chemin.suffix == ".ini" else CppLexer()
    fmt = HtmlFormatter(noclasses=True, nowrap=True, style="default")
    corps = highlight(texte, lexer, fmt)
    rel = chemin.relative_to(TEST)
    return f'<h3 class="fichier">{e(str(rel))}</h3><pre class="code">{corps}</pre>'


def annexes():
    fichiers = [TEST / "platformio.ini", TEST / "include" / "pinout.h", TEST / "src" / "main.cpp"]
    for nom in ["relais", "mq", "bh1750", "bmp280", "rfid", "tft", "menu"]:
        fichiers += [TEST / "include" / f"module_{nom}.h", TEST / "src" / f"module_{nom}.cpp"]
    code = "".join(bloc_code(f) for f in fichiers)
    return f"""
{h(1, "annexes")}
<table class="sommaire"><tr class="n1"><td class="t"><span>Annexe 1 : code du programme de test
(projet PlatformIO GouGouGaGAH_DomotiqueESP32)</span></td><td class="p">II</td></tr></table>
<div class="saut"></div>
{h(2, "a1")}
<p>Le programme de test est un projet PlatformIO (framework Arduino) distinct du programme final. Il initialise
chaque module, affiche son état sur le moniteur série et propose un menu tactile pour tester chaque fonction de la
carte (cf. 4.5). Le brochage est défini dans <span class="mono">pinout.h</span>, conforme au schéma structurel.</p>
{code}
"""


def quatrieme():
    return """
<div class="qcouv">
<h2>Résumé</h2>
<p>Ce dossier de fabrication accompagne la carte mSeguier de l'assistant domotique « Yuki », réalisée dans la
SAÉ 3.ESE.01-02 du BUT GEII. Il présente le schéma structurel de la carte, la nomenclature des composants, le
routage du circuit imprimé double face de 170 × 90 mm et les fichiers de fabrication transmis au fabricant
JLCPCB. Il décrit ensuite la procédure et le protocole de test de la carte : contrôles hors tension, mesure des
alimentations, programmation, vérification du bus I2C, des capteurs, du lecteur RFID, de l'écran tactile et de
la commande du relais, avec des limites d'acceptation chiffrées et un guide de diagnostic. Une fiche
d'intervention permet de consigner chaque contrôle ou réparation.</p>
<p class="motscles"><b>Mots-clés :</b> ESP32-C6, circuit imprimé, KiCad, procédure de test, maintenance</p>
<h2>Abstract</h2>
<p>This manufacturing file covers the mSeguier board of the “Yuki” home automation assistant. It gives the
schematic, the bill of materials, the layout of the 170 × 90 mm two-layer board and the Gerber files sent to
JLCPCB. It then describes the board test procedure, from unpowered checks to functional tests run with a
dedicated test program, with numeric pass limits, a troubleshooting guide and an intervention sheet.</p>
<p class="motscles"><b>Keywords:</b> ESP32-C6, printed circuit board, KiCad, test procedure, maintenance</p>
</div>"""


# ------------------------------------------------------------------ rendu PDF ----

def html_doc(corps):
    return f'<!doctype html><html lang="fr"><head><meta charset="utf-8"><style>{CSS}</style></head><body>{typo(corps)}</body></html>'


RENDU_JS = r"""
const { chromium } = require('playwright');
(async () => {
  const jobs = JSON.parse(process.argv[2]);
  const b = await chromium.launch();
  const p = await b.newPage();
  for (const [src, dst] of jobs) {
    await p.goto('file://' + src, { waitUntil: 'load' });
    await p.pdf({ path: dst, preferCSSPageSize: true, printBackground: true });
  }
  await b.close();
})();
"""


def rendre(tmp, morceaux):
    js = tmp / "rendu.js"
    js.write_text(RENDU_JS)
    jobs = []
    for nom, corps in morceaux:
        src = tmp / f"{nom}.html"
        src.write_text(html_doc(corps), encoding="utf-8")
        jobs.append([str(src), str(tmp / f"{nom}.pdf")])
    subprocess.run(["node", str(js), json.dumps(jobs)], check=True)
    return {nom: tmp / f"{nom}.pdf" for nom, _ in morceaux}


def romain(n):
    vals = [(10, "X"), (9, "IX"), (5, "V"), (4, "IV"), (1, "I")]
    r = ""
    for v, s in vals:
        while n >= v:
            r += s
            n -= v
    return r


def pages_des_titres(pdf, decalage):
    """Numéro de page (corps paginé à partir de 1) de chaque titre du sommaire."""
    n = len(PdfReader(str(pdf)).pages)
    textes = []
    for i in range(1, n + 1):
        t = subprocess.run(["pdftotext", "-f", str(i), "-l", str(i), "-layout", str(pdf), "-"],
                           capture_output=True, text=True).stdout
        textes.append(re.sub(r"\s+", " ", t))
    res = {}
    for niv, ancre, titre in SECTIONS:
        if ancre in ("annexes", "a1"):
            continue
        cible = re.sub(r"\s+", " ", titre)
        for i, t in enumerate(textes):
            if i == 0:
                continue  # page du sommaire
            if cible in t:
                res[ancre] = i + decalage
                break
    return res


def numeroter(entree, numeros, sortie_writer):
    pdfmetrics.registerFont(TTFont("Carlito", "/usr/share/fonts/truetype/crosextra/Carlito-Regular.ttf"))
    r = PdfReader(str(entree))
    for page, num in zip(r.pages, numeros):
        if num:
            w = float(page.mediabox.width)
            hh = float(page.mediabox.height)
            buf = io.BytesIO()
            c = canvas.Canvas(buf, pagesize=(w, hh))
            c.setFont("Carlito", 9)
            c.setFillColorRGB(0.35, 0.35, 0.35)
            c.drawString(54, 30, PIED)
            c.drawRightString(w - 54, 30, num)
            c.setStrokeColorRGB(0.6, 0.65, 0.75)
            c.line(54, 42, w - 54, 42)
            c.save()
            buf.seek(0)
            page.merge_page(PdfReader(buf).pages[0])
        sortie_writer.add_page(page)


def main():
    global N
    with tempfile.TemporaryDirectory() as d:
        tmp = pathlib.Path(d)

        def corps(pages):
            global N
            N = Num()
            return (sommaire(pages) + introduction() + partie_schema() + partie_nomenclature()
                    + partie_routage() + partie_procedure() + partie_fiche() + conclusion())

        # 1er passage : position des titres ; 2e passage : sommaire avec numéros
        p1 = rendre(tmp, [("corps", corps({}))])["corps"]
        pages = pages_des_titres(p1, 1)
        nb_corps = len(PdfReader(str(p1)).pages)
        pages["annexes"] = "I"
        pages["a1"] = "II"
        r = rendre(tmp, [("couv", couverture()), ("corps", corps(pages)),
                         ("annexes", annexes()), ("quatrieme", quatrieme())])
        pages2 = pages_des_titres(r["corps"], 1)
        if pages2 != {k: v for k, v in pages.items() if k not in ("annexes", "a1")}:
            print("Attention : pagination modifiée au 2e passage", pages, pages2)

        w = PdfWriter()
        numeroter(r["couv"], [None], w)
        nc = len(PdfReader(str(r["corps"])).pages)
        numeroter(r["corps"], [str(i) for i in range(1, nc + 1)], w)
        na = len(PdfReader(str(r["annexes"])).pages)
        numeroter(r["annexes"], [romain(i) for i in range(1, na + 1)], w)
        numeroter(r["quatrieme"], [None], w)
        w.add_metadata({"/Title": "Dossier de fabrication - Assistant domotique Yuki", "/Author": AUTEUR})
        with open(SORTIE, "wb") as f:
            w.write(f)
        print(f"{SORTIE} : {1 + nc + na + 1} pages (corps {nc}, annexes {na})")


if __name__ == "__main__":
    main()
