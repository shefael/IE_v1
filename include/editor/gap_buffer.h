#ifndef EDITOR_GAP_BUFFER_H
#define EDITOR_GAP_BUFFER_H

#include <stddef.h>

/* Structure opaque — la définition interne est dans gap_buffer.c */
typedef struct GapBuffer GapBuffer;

/*
 * Crée un nouveau gap buffer avec une capacité initiale donnée.
 * Retourne NULL en cas d'échec d'allocation.
 */
GapBuffer* gb_create(size_t capacite_initiale);

/*
 * Libère toute la mémoire associée au gap buffer.
 */
void gb_destroy(GapBuffer* gb);

/*
 * Insère 'len' octets de 'texte' à la position courante du curseur.
 * Retourne 0 en cas de succès, -1 en cas d'erreur mémoire.
 */
int gb_insert(GapBuffer* gb, const char* texte, size_t len);

/*
 * Supprime 'len' octets à gauche du curseur (équivalent Backspace).
 * Ne fait rien si le curseur est au début.
 */
void gb_delete(GapBuffer* gb, size_t len);

/*
 * Déplace le curseur à la position absolue 'pos' (en octets depuis le début).
 * Si pos dépasse la longueur, le curseur est placé à la fin.
 */
void gb_move_cursor(GapBuffer* gb, size_t pos);

/*
 * Retourne le curseur courant (position absolue en octets).
 */
size_t gb_get_cursor(const GapBuffer* gb);

/*
 * Alloue et retourne une copie plate du texte complet (sans le gap).
 * L'appelant doit libérer la mémoire avec free().
 * Retourne NULL en cas d'erreur.
 */
char* gb_get_text(const GapBuffer* gb);

/*
 * Retourne la longueur totale du texte en octets (sans le gap).
 */
size_t gb_get_length(const GapBuffer* gb);

/*
 * Remplace la plage [start, start+len[ par 'remplacement' (repl_len octets).
 * Opération atomique pour l'historique undo/redo.
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int gb_replace_range(GapBuffer* gb, size_t start, size_t len,
                     const char* remplacement, size_t repl_len);

#endif /* EDITOR_GAP_BUFFER_H */