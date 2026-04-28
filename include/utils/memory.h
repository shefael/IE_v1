#ifndef UTILS_MEMORY_H
#define UTILS_MEMORY_H

#include <stddef.h>

/*
 * Allocateur mémoire instrumenté pour IntelliEditor.
 *
 * En mode DEBUG (#define DEBUG ou cmake -DCMAKE_BUILD_TYPE=Debug) :
 *   - Chaque allocation est tracée (fichier, ligne, taille).
 *   - Des "canaris" (red zones) sont placés avant/après chaque bloc
 *     pour détecter les débordements (buffer overrun/underrun).
 *   - memory_rapport_fuites() signale les blocs non libérés à la sortie.
 *   - memory_valider_tout() vérifie l'intégrité des canaris de tous les blocs.
 *
 * En mode RELEASE :
 *   - Toutes les macros se résolvent vers malloc/free/realloc/strdup standards.
 *   - Aucune surcharge mémoire ni ralentissement.
 */

#ifdef DEBUG

/* --- API interne (ne pas appeler directement) --- */
void* memory_debug_malloc (size_t taille, const char* fichier, int ligne);
void* memory_debug_realloc(void* ptr, size_t taille, const char* fichier, int ligne);
char* memory_debug_strdup (const char* src, const char* fichier, int ligne);
void  memory_debug_free   (void* ptr, const char* fichier, int ligne);

/* --- Macros publiques --- */
#define ie_malloc(taille)       memory_debug_malloc((taille), __FILE__, __LINE__)
#define ie_realloc(ptr, taille) memory_debug_realloc((ptr), (taille), __FILE__, __LINE__)
#define ie_strdup(src)          memory_debug_strdup((src), __FILE__, __LINE__)
#define ie_free(ptr)            memory_debug_free((ptr), __FILE__, __LINE__)

/* --- Rapport et validation --- */
void memory_rapport_fuites(void);
void memory_valider_tout(void);
void memory_rapport_stats(void);

#else /* mode RELEASE */

#include <stdlib.h>
#include <string.h>

#define ie_malloc(taille)       malloc(taille)
#define ie_realloc(ptr, taille) realloc((ptr), (taille))
#define ie_strdup(src)          strdup(src)
#define ie_free(ptr)            free(ptr)

/* En release, ces fonctions ne font rien */
static inline void memory_rapport_fuites(void) {}
static inline void memory_valider_tout(void)   {}
static inline void memory_rapport_stats(void)  {}

#endif /* DEBUG */

#endif /* UTILS_MEMORY_H */