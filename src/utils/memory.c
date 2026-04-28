#include "utils/memory.h"

#ifdef DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Valeur des canaris placés en bordure de chaque allocation.
 * Si ces octets changent, un débordement a eu lieu.
 */
#define CANARI_VALEUR  0xDE
#define CANARI_TAILLE  8   /* octets de garde avant ET après le bloc */

/*
 * En-tête stocké AVANT chaque bloc alloué par l'utilisateur.
 * Disposition mémoire réelle :
 *
 *   [EnteteBloc] [CANARI x8] [données utilisateur] [CANARI x8]
 *
 * Le pointeur retourné à l'utilisateur pointe sur "données utilisateur".
 */
typedef struct EnteteBloc {
    struct EnteteBloc* suivant;
    struct EnteteBloc* precedent;
    size_t             taille;       /* taille demandée par l'utilisateur */
    const char*        fichier;      /* __FILE__ de l'appel */
    int                ligne;        /* __LINE__ de l'appel */
    int                libere;       /* 1 si ie_free() a déjà été appelé */
} EnteteBloc;

/* Liste doublement chaînée de tous les blocs actifs */
static EnteteBloc* g_tete    = NULL;
static size_t      g_nb_alloc_actives = 0;
static size_t      g_pic_memoire      = 0;
static size_t      g_memoire_actuelle = 0;
static size_t      g_total_allocations = 0;

/* Pointeur vers la zone données utilisateur depuis l'en-tête */
static void* entete_vers_donnees(EnteteBloc* e) {
    return (unsigned char*)e + sizeof(EnteteBloc) + CANARI_TAILLE;
}

/* Pointeur vers l'en-tête depuis la zone données utilisateur */
static EnteteBloc* donnees_vers_entete(void* ptr) {
    return (EnteteBloc*)((unsigned char*)ptr - CANARI_TAILLE - sizeof(EnteteBloc));
}

/* Écrit les canaris autour du bloc utilisateur */
static void ecrire_canaris(EnteteBloc* e) {
    unsigned char* canari_avant = (unsigned char*)e + sizeof(EnteteBloc);
    unsigned char* canari_apres = canari_avant + CANARI_TAILLE + e->taille;
    memset(canari_avant, CANARI_VALEUR, CANARI_TAILLE);
    memset(canari_apres, CANARI_VALEUR, CANARI_TAILLE);
}

/* Vérifie l'intégrité des canaris d'un bloc. Retourne 1 si OK, 0 si corrompu. */
static int verifier_canaris(EnteteBloc* e) {
    unsigned char* canari_avant = (unsigned char*)e + sizeof(EnteteBloc);
    unsigned char* canari_apres = canari_avant + CANARI_TAILLE + e->taille;
    for (int i = 0; i < CANARI_TAILLE; i++) {
        if (canari_avant[i] != CANARI_VALEUR) return 0;
        if (canari_apres[i] != CANARI_VALEUR) return 0;
    }
    return 1;
}

/* Insère un bloc en tête de la liste chaînée */
static void inserer_bloc(EnteteBloc* e) {
    e->suivant   = g_tete;
    e->precedent = NULL;
    if (g_tete) g_tete->precedent = e;
    g_tete = e;

    g_nb_alloc_actives++;
    g_total_allocations++;
    g_memoire_actuelle += e->taille;
    if (g_memoire_actuelle > g_pic_memoire) g_pic_memoire = g_memoire_actuelle;
}

/* Retire un bloc de la liste chaînée */
static void retirer_bloc(EnteteBloc* e) {
    if (e->precedent) e->precedent->suivant = e->suivant;
    else              g_tete                = e->suivant;
    if (e->suivant)   e->suivant->precedent = e->precedent;

    g_nb_alloc_actives--;
    g_memoire_actuelle -= e->taille;
}

/* ------------------------------------------------------------------ */

void* memory_debug_malloc(size_t taille, const char* fichier, int ligne) {
    if (taille == 0) taille = 1; /* malloc(0) comportement indéfini */

    /* Taille totale : en-tête + canari avant + données + canari après */
    size_t taille_totale = sizeof(EnteteBloc) + CANARI_TAILLE + taille + CANARI_TAILLE;
    EnteteBloc* e = (EnteteBloc*)malloc(taille_totale);
    if (!e) {
        fprintf(stderr, "[MEMORY] ÉCHEC malloc(%zu) à %s:%d\n", taille, fichier, ligne);
        return NULL;
    }

    e->taille  = taille;
    e->fichier = fichier;
    e->ligne   = ligne;
    e->libere  = 0;

    ecrire_canaris(e);
    inserer_bloc(e);

    void* ptr = entete_vers_donnees(e);
    /* Initialise à 0xAA pour détecter les lectures sans initialisation */
    memset(ptr, 0xAA, taille);
    return ptr;
}

void* memory_debug_realloc(void* ptr, size_t taille, const char* fichier, int ligne) {
    if (!ptr) return memory_debug_malloc(taille, fichier, ligne);
    if (taille == 0) {
        memory_debug_free(ptr, fichier, ligne);
        return NULL;
    }

    EnteteBloc* e = donnees_vers_entete(ptr);

    /* Vérifie les canaris avant de réallouer */
    if (!verifier_canaris(e)) {
        fprintf(stderr, "[MEMORY] DÉBORDEMENT détecté avant realloc(%zu) à %s:%d\n"
                        "         Allocation originale : %s:%d (%zu octets)\n",
                taille, fichier, ligne, e->fichier, e->ligne, e->taille);
    }

    retirer_bloc(e);

    size_t taille_totale = sizeof(EnteteBloc) + CANARI_TAILLE + taille + CANARI_TAILLE;
    EnteteBloc* nouveau = (EnteteBloc*)realloc(e, taille_totale);
    if (!nouveau) {
        fprintf(stderr, "[MEMORY] ÉCHEC realloc(%zu) à %s:%d\n", taille, fichier, ligne);
        inserer_bloc(e); /* réinscrit l'ancien bloc */
        return NULL;
    }

    nouveau->taille  = taille;
    nouveau->fichier = fichier;
    nouveau->ligne   = ligne;
    nouveau->libere  = 0;

    /* Remet à jour les pointeurs dans la liste chaînée */
    if (nouveau->suivant)   nouveau->suivant->precedent = nouveau;
    if (nouveau->precedent) nouveau->precedent->suivant = nouveau;
    else                    g_tete = nouveau;

    ecrire_canaris(nouveau);

    g_nb_alloc_actives++;
    g_memoire_actuelle += taille;
    if (g_memoire_actuelle > g_pic_memoire) g_pic_memoire = g_memoire_actuelle;

    return entete_vers_donnees(nouveau);
}

char* memory_debug_strdup(const char* src, const char* fichier, int ligne) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char* copie = (char*)memory_debug_malloc(len, fichier, ligne);
    if (copie) memcpy(copie, src, len);
    return copie;
}

void memory_debug_free(void* ptr, const char* fichier, int ligne) {
    if (!ptr) return; /* free(NULL) est légal, on l'ignore */

    EnteteBloc* e = donnees_vers_entete(ptr);

    /* Double free ? */
    if (e->libere) {
        fprintf(stderr, "[MEMORY] DOUBLE FREE détecté à %s:%d\n"
                        "         Allocation originale : %s:%d (%zu octets)\n",
                fichier, ligne, e->fichier, e->ligne, e->taille);
        return;
    }

    /* Canaris corrompus ? */
    if (!verifier_canaris(e)) {
        fprintf(stderr, "[MEMORY] DÉBORDEMENT détecté lors du free à %s:%d\n"
                        "         Allocation originale : %s:%d (%zu octets)\n",
                fichier, ligne, e->fichier, e->ligne, e->taille);
    }

    e->libere = 1;
    retirer_bloc(e);

    /* Écrase les données avec 0xDD pour détecter les use-after-free */
    memset(entete_vers_donnees(e), 0xDD, e->taille);
    free(e);
}

void memory_rapport_fuites(void) {
    if (!g_tete) {
        fprintf(stderr, "[MEMORY] Aucune fuite détectée. ✓\n");
        return;
    }
    fprintf(stderr, "[MEMORY] === FUITES MÉMOIRE DÉTECTÉES ===\n");
    EnteteBloc* e = g_tete;
    int nb = 0;
    while (e) {
        fprintf(stderr, "  [FUITE #%d] %zu octets — alloué à %s:%d\n",
                ++nb, e->taille, e->fichier, e->ligne);
        e = e->suivant;
    }
    fprintf(stderr, "[MEMORY] Total : %d bloc(s) non libéré(s), %zu octets\n",
            nb, g_memoire_actuelle);
}

void memory_valider_tout(void) {
    EnteteBloc* e = g_tete;
    int nb_erreurs = 0;
    while (e) {
        if (!verifier_canaris(e)) {
            fprintf(stderr, "[MEMORY] DÉBORDEMENT sur bloc alloué à %s:%d (%zu octets)\n",
                    e->fichier, e->ligne, e->taille);
            nb_erreurs++;
        }
        e = e->suivant;
    }
    if (nb_erreurs == 0)
        fprintf(stderr, "[MEMORY] Validation OK — %zu bloc(s) actif(s), aucun débordement.\n",
                g_nb_alloc_actives);
}

void memory_rapport_stats(void) {
    fprintf(stderr, "[MEMORY] === STATISTIQUES ===\n");
    fprintf(stderr, "  Allocations totales   : %zu\n", g_total_allocations);
    fprintf(stderr, "  Blocs actifs actuels  : %zu\n", g_nb_alloc_actives);
    fprintf(stderr, "  Mémoire actuelle      : %zu octets\n", g_memoire_actuelle);
    fprintf(stderr, "  Pic mémoire           : %zu octets\n", g_pic_memoire);
}

#endif /* DEBUG */