#ifndef MODULE_TEMPO_H
#define MODULE_TEMPO_H

#include <Arduino.h>

// Couleur Tempo EDF du jour et du lendemain, affichée sur la LED RGB de la carte.
// La LED clignote en permanence : preuve que le programme tourne.

enum CouleurTempo { TEMPO_INCONNU = 0, TEMPO_BLEU = 1, TEMPO_BLANC = 2, TEMPO_ROUGE = 3 };

void tempo_setup();

// À appeler à chaque tour de loop() : fait clignoter la LED et
// met à jour les couleurs toutes les heures (quand le WiFi est là).
void tempo_loop();

CouleurTempo tempo_aujourdhui();
CouleurTempo tempo_demain();

// "bleu", "blanc", "rouge" ou "inconnu"
const char *tempo_nom(CouleurTempo c);

#endif // MODULE_TEMPO_H
