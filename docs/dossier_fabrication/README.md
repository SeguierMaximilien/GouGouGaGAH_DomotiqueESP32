# Dossier de fabrication (SAE3.ESE.01-02 AssistDomo, rendu semaine 2)

- `Dossier_fabrication_AssistDomo_SEGUIER.pdf` : le dossier à rendre (un seul PDF) : schéma structurel, nomenclature,
  routage, procédure et protocole de test, fiche d'intervention, annexe code de test.
- `generer_dossier_fabrication.py` : régénère le PDF (`python3 docs/dossier_fabrication/generer_dossier_fabrication.py`
  depuis la racine ; nécessite pypdf, reportlab, pygments, node + playwright, pdftotext).
- `figures/` : schéma et couches cuivre exportés par `kicad-cli` depuis `mSeguier/`, rendu 3D rogné.
  À ré-exporter si le schéma ou le PCB change :
  `kicad-cli sch export svg -o figures/sch --no-background-color mSeguier/mSeguier.kicad_sch`
  `kicad-cli pcb export svg --mode-single --layers F.Cu,Edge.Cuts,F.Silkscreen --exclude-drawing-sheet --page-size-mode 2 -o figures/pcb_dessus.svg mSeguier/mSeguier.kicad_pcb`
  (idem `B.Cu,Edge.Cuts,B.Silkscreen --mirror` pour `pcb_dessous.svg`).
- `logos/` (facultatif) : déposer les logos IUT de Toulon / Université de Toulon en PNG ; ils remplacent le texte en haut de la couverture.

Mise en page selon les « Normes de présentation d'un rapport scientifique ou d'un document technique » (BUT GEII) et
fiche d'intervention sur le modèle de l'exemple fourni sur Moodle.
