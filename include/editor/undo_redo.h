#ifndef EDITOR_UNDO_REDO_H
#define EDITOR_UNDO_REDO_H

#include <stddef.h>
#include "editor/gap_buffer.h"

/*
 * Pattern Command appliqué à l'édition de texte.
 *
 * Chaque commande encapsule :
 *   - l'opération à exécuter (execute)
 *   - l'opération inverse (undo)
 *   - les données nécessaires (position, texte, longueur)
 *   - un pointeur de libération mémoire (destroy)
 */

/* Type de commande */
typedef enum {
    CMD_INSERT,  /* Insertion de texte */
    CMD_DELETE   /* Suppression de texte */
} CommandType;

/* Structure d'une commande */
typedef struct Command {
    CommandType type;
    size_t      position;   /* Position absolue dans le buffer (en octets) */
    char*       texte;      /* Texte inséré ou supprimé (copie allouée)    */
    size_t      longueur;   /* Longueur en octets du texte                 */
} Command;

/*
 * Historique undo/redo.
 * Implémenté comme deux piles : pile undo et pile redo.
 * Capacité maximale configurable (HISTORIQUE_MAX_COMMANDES).
 */
typedef struct History History;

/* Capacité maximale de l'historique (nombre de commandes) */
#define HISTORIQUE_MAX_COMMANDES 512

/*
 * Crée un nouvel historique vide.
 * Retourne NULL en cas d'échec.
 */
History* history_create(void);

/*
 * Libère toute la mémoire de l'historique.
 */
void history_destroy(History* h);

/*
 * Crée et exécute une commande d'insertion.
 * Enregistre la commande dans la pile undo.
 * Vide la pile redo (un nouvel acte invalide le futur).
 */
int history_execute_insert(History* h, GapBuffer* gb,
                           size_t position, const char* texte, size_t longueur);

/*
 * Crée et exécute une commande de suppression.
 * Le texte supprimé doit être fourni (il faut le sauvegarder avant suppression).
 */
int history_execute_delete(History* h, GapBuffer* gb,
                           size_t position, const char* texte, size_t longueur);

/*
 * Crée et exécute une commande de remplacement atomique.
 * Combine suppression + insertion en une seule entrée d'historique.
 * Un seul Ctrl+Z suffit pour annuler.
 */
int history_execute_replace(History* h, GapBuffer* gb,
                            size_t position,
                            const char* ancien_texte, size_t ancien_len,
                            const char* nouveau_texte, size_t nouveau_len);

/*
 * Annule la dernière commande (Ctrl+Z).
 * Retourne 0 si une commande a été annulée, -1 si la pile est vide.
 */
int history_undo(History* h, GapBuffer* gb);

/*
 * Rétablit la dernière commande annulée (Ctrl+Y / Ctrl+Shift+Z).
 * Retourne 0 si une commande a été rétablie, -1 si la pile est vide.
 */
int history_redo(History* h, GapBuffer* gb);

/*
 * Vide les deux piles (utilisé lors d'un "Nouveau document").
 */
void history_clear(History* h);

/*
 * Retourne le nombre de commandes dans la pile undo.
 */
size_t history_undo_count(const History* h);

/*
 * Retourne le nombre de commandes dans la pile redo.
 */
size_t history_redo_count(const History* h);

#endif /* EDITOR_UNDO_REDO_H */