#ifndef MODULE_ACCES_H
#define MODULE_ACCES_H

#include <Arduino.h>

// Contrôle d'accès : badges autorisés (mémorisés en NVS) et gâche de porte (relais).

// ACCES_VALIDATION : avant d'ajouter un badge, il faut d'abord passer un badge déjà autorisé.
enum ModeAcces { ACCES_NORMAL, ACCES_VALIDATION, ACCES_AJOUT, ACCES_SUPPRESSION };

// Charge la liste des badges autorisés depuis la mémoire NVS.
void acces_setup();

// À appeler à chaque tour de loop() : lit les badges et referme la porte après le délai.
void acces_loop();

// Ouvre la porte (relais collé quelques secondes).
void acces_ouvrirPorte();
bool acces_porteOuverte();

// Le prochain badge présenté sera ajouté / supprimé (15 s pour le présenter).
// Pour l'ajout, il faut d'abord passer un badge déjà autorisé (sauf si la liste est vide).
void acces_demanderAjout();
void acces_demanderSuppression();
void acces_annulerMode();
ModeAcces acces_mode();

// Efface tous les badges autorisés.
void acces_toutEffacer();

int acces_nbBadges();

// Dernier événement lisible (ex : "04 A3 F1 2B : autorise").
String acces_dernierMessage();

#endif // MODULE_ACCES_H
