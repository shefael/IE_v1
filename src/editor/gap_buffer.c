#include "editor/gap_buffer.h"

#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Principe du gap buffer :
 *
 *   [ texte_avant_curseur | --- GAP --- | texte_après_curseur ]
 *   ^                     ^             ^                     ^
 *   buffer                debut_gap     fin_gap               buffer+capacite
 *
 * Le gap est un espace vide maintenu autour du curseur.
 * Les insertions sont O(1) tant que le gap n'est pas plein.
 * Un déplacement du curseur copie du texte pour combler/rouvrir le gap.
 * ----------------------------------------------------------------------- */

struct GapBuffer {
    char*  buffer;      /* Bloc mémoire complet                    */
    size_t capacite;    /* Taille totale du bloc                   */
    size_t debut_gap;   /* Index du premier octet du gap           */
    size_t fin_gap;     /* Index du premier octet après le gap     */
};

/* Taille minimale du gap à maintenir lors d'un redimensionnement */
#define TAILLE_GAP_MIN 64

/* Facteur de croissance géométrique */
#define FACTEUR_CROISSANCE 2

/* -----------------------------------------------------------------------
 * Fonctions internes
 * ----------------------------------------------------------------------- */

/*
 * Agrandit le buffer pour que le gap soit d'au moins 'besoin' octets.
 * Retourne 0 en cas de succès, -1 en cas d'échec malloc.
 */
static int gb_agrandir(GapBuffer* gb, size_t besoin) {
    size_t taille_gap_actuelle = gb->fin_gap - gb->debut_gap;

    if (taille_gap_actuelle >= besoin) {
        return 0; /* Déjà assez de place */
    }

    /* Calcul de la nouvelle capacité par doublement */
    size_t texte_len = gb->capacite - taille_gap_actuelle;
    size_t nouvelle_capacite = (gb->capacite + besoin) * FACTEUR_CROISSANCE;
    if (nouvelle_capacite < texte_len + besoin + TAILLE_GAP_MIN) {
        nouvelle_capacite = texte_len + besoin + TAILLE_GAP_MIN;
    }

    char* nouveau_buffer = (char*)malloc(nouvelle_capacite);
    if (!nouveau_buffer) {
        return -1;
    }

    /* Copie du texte avant le gap */
    memcpy(nouveau_buffer, gb->buffer, gb->debut_gap);

    /* Calcul de la longueur du texte après le gap */
    size_t len_apres = gb->capacite - gb->fin_gap;

    /* Nouvelle position de fin de gap */
    size_t nouvelle_fin_gap = nouvelle_capacite - len_apres;

    /* Copie du texte après le gap à la fin du nouveau buffer */
    memcpy(nouveau_buffer + nouvelle_fin_gap,
           gb->buffer + gb->fin_gap,
           len_apres);

    free(gb->buffer);
    gb->buffer    = nouveau_buffer;
    gb->fin_gap   = nouvelle_fin_gap;
    gb->capacite  = nouvelle_capacite;

    return 0;
}

/* -----------------------------------------------------------------------
 * API publique
 * ----------------------------------------------------------------------- */

GapBuffer* gb_create(size_t capacite_initiale) {
    if (capacite_initiale < TAILLE_GAP_MIN) {
        capacite_initiale = TAILLE_GAP_MIN;
    }

    GapBuffer* gb = (GapBuffer*)malloc(sizeof(GapBuffer));
    if (!gb) return NULL;

    gb->buffer = (char*)malloc(capacite_initiale);
    if (!gb->buffer) {
        free(gb);
        return NULL;
    }

    gb->capacite  = capacite_initiale;
    gb->debut_gap = 0;                  /* Curseur au début   */
    gb->fin_gap   = capacite_initiale;  /* Gap occupe tout    */

    return gb;
}

void gb_destroy(GapBuffer* gb) {
    if (!gb) return;
    free(gb->buffer);
    free(gb);
}

int gb_insert(GapBuffer* gb, const char* texte, size_t len) {
    if (!gb || !texte || len == 0) return 0;

    /* Agrandir si le gap est trop petit */
    if (gb->fin_gap - gb->debut_gap < len) {
        if (gb_agrandir(gb, len) != 0) {
            return -1;
        }
    }

    /* Insertion dans le gap */
    memcpy(gb->buffer + gb->debut_gap, texte, len);
    gb->debut_gap += len;

    return 0;
}

void gb_delete(GapBuffer* gb, size_t len) {
    if (!gb || len == 0) return;

    /* On ne peut pas supprimer plus qu'il n'y a avant le curseur */
    if (len > gb->debut_gap) {
        len = gb->debut_gap;
    }

    /* Agrandir le gap vers la gauche = reculer debut_gap */
    gb->debut_gap -= len;
}

void gb_move_cursor(GapBuffer* gb, size_t pos) {
    if (!gb) return;

    size_t longueur = gb_get_length(gb);
    if (pos > longueur) {
        pos = longueur;
    }

    if (pos < gb->debut_gap) {
        /* Déplacement vers la gauche :
         * on déplace du texte depuis avant le gap vers après le gap */
        size_t delta = gb->debut_gap - pos;
        gb->fin_gap   -= delta;
        gb->debut_gap  = pos;
        memmove(gb->buffer + gb->fin_gap,
                gb->buffer + gb->debut_gap,
                delta);
    } else if (pos > gb->debut_gap) {
        /* Déplacement vers la droite :
         * pos est en coordonnées "texte plat", il faut convertir
         * en coordonnées buffer (après le gap) */
        size_t delta = pos - gb->debut_gap;
        /* Les octets à déplacer sont dans la zone après le gap */
        memmove(gb->buffer + gb->debut_gap,
                gb->buffer + gb->fin_gap,
                delta);
        gb->debut_gap += delta;
        gb->fin_gap   += delta;
    }
    /* Si pos == debut_gap, rien à faire */
}

size_t gb_get_cursor(const GapBuffer* gb) {
    if (!gb) return 0;
    return gb->debut_gap;
}

char* gb_get_text(const GapBuffer* gb) {
    if (!gb) return NULL;

    size_t len_avant  = gb->debut_gap;
    size_t len_apres  = gb->capacite - gb->fin_gap;
    size_t longueur   = len_avant + len_apres;

    char* resultat = (char*)malloc(longueur + 1);
    if (!resultat) return NULL;

    /* Copie des deux moitiés du texte (avant et après le gap) */
    memcpy(resultat, gb->buffer, len_avant);
    memcpy(resultat + len_avant, gb->buffer + gb->fin_gap, len_apres);
    resultat[longueur] = '\0';

    return resultat;
}

size_t gb_get_length(const GapBuffer* gb) {
    if (!gb) return 0;
    /* Longueur = capacité totale - taille du gap */
    return gb->capacite - (gb->fin_gap - gb->debut_gap);
}

int gb_replace_range(GapBuffer* gb, size_t start, size_t len,
                     const char* remplacement, size_t repl_len) {
    if (!gb) return -1;

    size_t longueur = gb_get_length(gb);

    /* Clamp de la plage */
    if (start > longueur) start = longueur;
    if (start + len > longueur) len = longueur - start;

    /* 1. Déplacer le curseur au début de la plage */
    gb_move_cursor(gb, start);

    /* 2. Supprimer 'len' octets après le curseur
     *    (ils sont maintenant juste après le gap = gb->fin_gap) */
    if (len > 0) {
        size_t disponible = gb->capacite - gb->fin_gap;
        if (len > disponible) len = disponible;
        gb->fin_gap += len;
    }

    /* 3. Insérer le texte de remplacement */
    if (repl_len > 0) {
        return gb_insert(gb, remplacement, repl_len);
    }

    return 0;
}