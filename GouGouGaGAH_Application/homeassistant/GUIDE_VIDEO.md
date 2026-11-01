# Vidéo de preuve Home Assistant : mise en place + script de tournage

À rendre ce soir avant 23h59, fichier vidéo de 250 Mo maximum.
Durée visée : 3 à 4 minutes. Un téléphone en 1080p fait environ 100 Mo pour 3 min, ça passe.

## A. Préparation (environ 20 min, à faire tout de suite)

1. **Mettre le nouveau code sur l'ESP32** : dans GitHub Desktop, Fetch origin, puis Pull origin, puis Upload dans VS Code.
   Laisse ensuite la carte branchée et allumée : **les courbes ont besoin d'au moins 30 min de mesures**.
2. **Vérifier les noms d'entités** : Paramètres → Appareils et services → MQTT → GouGouGaGAH.
   Ils doivent correspondre à `entites.txt` (par ex. `sensor.gougougagah_temperature`).
   Si un nom diffère, remplace-le dans les fichiers YAML avant de les coller.
3. **Créer la consigne** : Paramètres → Appareils et services → onglet **Entrées** → Créer une entrée → **Nombre**.
   - Nom : `Temperature souhaitee`
   - Min 15, Max 30, Pas 0,5, unité `°C`, affichage curseur

   L'entité doit s'appeler `input_number.temperature_souhaitee`.
4. **Automatisations** : Paramètres → Automatisations et scènes → Créer une automatisation → Créer une nouvelle automatisation.
   - Ouvre ⋮ en haut à droite → **Modifier en YAML**.
   - Colle **un seul bloc** de `automations.yaml`, sans le tiret `- ` du début et en enlevant 2 espaces d'indentation.
   - Enregistre, puis recommence pour chacun des 5 blocs.
5. **Tableau de bord** : Paramètres → Tableaux de bord → Ajouter → Nouveau tableau de bord vide, nommé « AssistDomo ».
   - Ouvre-le, puis ✏️ (modifier) → ⋮ → **Éditeur de configuration brute**.
   - Colle tout `dashboard.yaml`, puis Enregistrer.
6. Côté PC : ouvre le moniteur série de PlatformIO (115200 bauds) et agrandis la police (Ctrl + molette).

## B. Mise en place pour filmer

- Sur l'écran du PC, côte à côte : le **navigateur** avec le tableau de bord AssistDomo, et le **moniteur série**.
- Sur la table, devant : la **carte ESP32** avec l'écran tactile, le lecteur RFID et le relais.
- Filme au téléphone en paysage. Passe du PC à la carte selon les plans, et parle pendant le tournage, ou ajoute du texte au montage.

## C. Script de tournage (plan par plan)

| # | Ce que tu montres | Ce que tu fais | Ce qu'on doit voir / dire |
|---|---|---|---|
| 1 | Vue d'ensemble (5 s) | Plan large sur la carte et le PC | « Projet AssistDomo : ESP32-C6 relié à Home Assistant en MQTT. » |
| 2 | Capteurs qui varient | **Main au-dessus du BH1750** pendant 15 s, puis retire-la | La luminosité chute sur le tableau de bord, puis remonte. La valeur change aussi sur l'écran tactile (menu Capteurs). |
| 3 | Automatisation n°1 : éclairage | Pendant le plan 2, quand la luminosité passe sous 30 lx | Le curseur **Éclairage** monte tout seul à 80 %, et le moniteur série affiche `>>> ORDRE RECU : ECLAIRAGE : allume...`. En retirant la main, il revient à 0 %. |
| 4 | Température | **Pince le BMP280 entre deux doigts** pendant 20 s | La température monte de 1 à 3 °C (une mise à jour toutes les 10 s). |
| 5 | Gaz | **Souffle** sur le capteur MQ, ou approche un peu de gel hydroalcoolique (sans flamme) | La valeur brute du gaz monte. |
| 6 | Courbes de tendance | Fais défiler jusqu'aux cartes « Température (24 h) », « Luminosité », « Gaz », puis clique sur une courbe | On voit l'historique, avec les pics des plans 2 à 5. |
| 7 | Bouton relais | Clique sur **Ouvrir la porte** dans HA, en filmant **la carte** | On entend le **clic du relais**. Le moniteur série affiche `[HA] Ordre : ouvrir la porte`, et la porte passe à « Ouvert » 3 s puis « Fermé ». |
| 8 | Badge RFID + automatisation n°2 | Passe un **badge autorisé**, puis un **badge inconnu** | Le relais claque pour le bon badge seulement. Dans HA, la **cloche de notifications** (en bas à gauche) affiche « Badge xx : autorise » puis « refuse ». Le moniteur série affiche `ORDRE RECU : NOTIF : badge ...`. |
| 9 | Automatisation n°3 : pergola / volets | Dans « Commandes », mets la **Température souhaitée** sous la température mesurée, puis au-dessus | Le moniteur série affiche `PERGOLA : lames fermees (automne, T=22.4 > 20.0)` puis `lames ouvertes + VOLETS ouverts`. Le même texte s'affiche sur l'écran tactile, menu **Infos**, ligne « Ordre ». |
| 10 | Les automatisations | Paramètres → Automatisations et scènes | Montre la liste des 5 automatisations AssistDomo, puis ouvre-en une pour montrer déclencheur, condition et action. |
| 11 | Bonus | Écran tactile : menu Infos (Tempo du jour / demain) + LED RGB qui clignote | « La LED affiche la couleur Tempo EDF du jour puis du lendemain. » |

## D. Si un plan rate

- **Rien ne bouge dans HA** : regarde le moniteur série. Il doit afficher `MQTT OK`. Sinon, attends 10 s : la reconnexion est automatique.
- **Une automatisation ne se déclenche pas** : Paramètres → Automatisations → l'automatisation → ⋮ → **Traces**, qui montre pourquoi.
- **La luminosité ne passe pas sous 30 lx** : couvre complètement le capteur avec le doigt.
- **La luminosité ne remonte pas au-dessus de 100 lx** : éclaire-le avec la lampe du téléphone.
- **Pas de courbe** : l'historique démarre quand la carte envoie ses premières mesures. Laisse tourner plus longtemps avant de filmer le plan 6.
