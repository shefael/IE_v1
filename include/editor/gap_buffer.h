#ifndef EDITOR_GAP_BUFFER_H
#define EDITOR_GAP_BUFFER_H

#include <stddef.h>   /* pour size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Structure opaque représentant le gap buffer */
typedef struct GapBuffer GapBuffer;

/* Crée un nouveau gap buffer avec une capacité initiale */
GapBuffer* gb_create(size_t initial_capacity);

/* Détruit un gap buffer et libère la mémoire */
void gb_destroy(GapBuffer* gb);

/* Insère du texte à la position courante du curseur */
int gb_insert(GapBuffer* gb, const char* text, size_t len);

/* Supprime un nombre de caractères avant ou après le curseur */
int gb_delete(GapBuffer* gb, size_t count, int forward);

/* Récupère le contenu complet sous forme de chaîne UTF-8 */
const char* gb_get_text(const GapBuffer* gb);

/* Obtient la longueur totale du texte (sans le gap) */
size_t gb_length(const GapBuffer* gb);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_GAP_BUFFER_H */