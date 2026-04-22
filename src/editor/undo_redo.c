#include "editor/undo_redo.h"

#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Structure interne d'une commande de remplacement atomique.
 * On stocke à la fois l'ancien et le nouveau texte pour pouvoir
 * undo/redo en une seule opération.
 * ----------------------------------------------------------------------- */
typedef struct {
    size_t position;
    char*  ancien_texte;
    size_t ancien_len;
    char*  nouveau_texte;
    size_t nouveau_len;
} CmdReplace;

/* -----------------------------------------------------------------------
 * Entrée générique dans la pile d'historique.
 * On distingue les commandes simples (insert/delete) des remplacements.
 * ----------------------------------------------------------------------- */
typedef enum {
    ENTREE_SIMPLE,   /* Command (insert ou delete) */
    ENTREE_REPLACE   /* CmdReplace (atomique)      */
} EntreeType;

typedef struct {
    EntreeType type;
    union {
        Command    simple;
        CmdReplace replace;
    } data;
} EntreeHistorique;

/* -----------------------------------------------------------------------
 * Structure de l'historique : deux tableaux de taille fixe.
 * ----------------------------------------------------------------------- */
struct History {
    EntreeHistorique pile_undo[HISTORIQUE_MAX_COMMANDES];
    int              top_undo; /* Index du sommet (-1 si vide) */

    EntreeHistorique pile_redo[HISTORIQUE_MAX_COMMANDES];
    int              top_redo;
};

/* -----------------------------------------------------------------------
 * Fonctions internes de libération
 * ----------------------------------------------------------------------- */

static void liberer_entree(EntreeHistorique* e) {
    if (!e) return;
    if (e->type == ENTREE_SIMPLE) {
        free(e->data.simple.texte);
        e->data.simple.texte = NULL;
    } else {
        free(e->data.replace.ancien_texte);
        free(e->data.replace.nouveau_texte);
        e->data.replace.ancien_texte = NULL;
        e->data.replace.nouveau_texte = NULL;
    }
}

static void vider_pile(EntreeHistorique* pile, int* top) {
    while (*top >= 0) {
        liberer_entree(&pile[*top]);
        (*top)--;
    }
}

/* -----------------------------------------------------------------------
 * API publique
 * ----------------------------------------------------------------------- */

History* history_create(void) {
    History* h = (History*)calloc(1, sizeof(History));
    if (!h) return NULL;
    h->top_undo = -1;
    h->top_redo = -1;
    return h;
}

void history_destroy(History* h) {
    if (!h) return;
    vider_pile(h->pile_undo, &h->top_undo);
    vider_pile(h->pile_redo, &h->top_redo);
    free(h);
}

void history_clear(History* h) {
    if (!h) return;
    vider_pile(h->pile_undo, &h->top_undo);
    vider_pile(h->pile_redo, &h->top_redo);
}

size_t history_undo_count(const History* h) {
    if (!h) return 0;
    return (size_t)(h->top_undo + 1);
}

size_t history_redo_count(const History* h) {
    if (!h) return 0;
    return (size_t)(h->top_redo + 1);
}

/* Ajoute une entrée dans la pile undo. Vide la pile redo. */
static int pousser_undo(History* h, EntreeHistorique* entree) {
    /* Vider le redo : un nouvel acte invalide le futur */
    vider_pile(h->pile_redo, &h->top_redo);

    /* Si la pile undo est pleine, on supprime la commande la plus ancienne
     * en décalant le tableau (stratégie FIFO sur dépassement) */
    if (h->top_undo >= HISTORIQUE_MAX_COMMANDES - 1) {
        liberer_entree(&h->pile_undo[0]);
        memmove(&h->pile_undo[0], &h->pile_undo[1],
                sizeof(EntreeHistorique) * (HISTORIQUE_MAX_COMMANDES - 1));
        h->top_undo--;
    }

    h->top_undo++;
    h->pile_undo[h->top_undo] = *entree;
    return 0;
}

int history_execute_insert(History* h, GapBuffer* gb,
                           size_t position, const char* texte, size_t longueur) {
    if (!h || !gb || !texte || longueur == 0) return -1;

    /* Copie du texte pour l'historique */
    char* copie = (char*)malloc(longueur + 1);
    if (!copie) return -1;
    memcpy(copie, texte, longueur);
    copie[longueur] = '\0';

    /* Exécution : déplacer le curseur puis insérer */
    gb_move_cursor(gb, position);
    if (gb_insert(gb, texte, longueur) != 0) {
        free(copie);
        return -1;
    }

    /* Enregistrement dans l'historique */
    EntreeHistorique entree;
    entree.type                = ENTREE_SIMPLE;
    entree.data.simple.type     = CMD_INSERT;
    entree.data.simple.position = position;
    entree.data.simple.texte    = copie;
    entree.data.simple.longueur = longueur;

    if (pousser_undo(h, &entree) != 0) {
        free(copie);
        return -1;
    }

    return 0;
}

int history_execute_delete(History* h, GapBuffer* gb,
                           size_t position, const char* texte, size_t longueur) {
    if (!h || !gb || longueur == 0) return -1;

    /* Copie du texte supprimé pour pouvoir le restaurer */
    char* copie = NULL;
    if (texte) {
        copie = (char*)malloc(longueur + 1);
        if (!copie) return -1;
        memcpy(copie, texte, longueur);
        copie[longueur] = '\0';
    }

    /* Exécution : curseur après la zone puis delete vers la gauche */
    gb_move_cursor(gb, position + longueur);
    gb_delete(gb, longueur);

    /* Enregistrement */
    EntreeHistorique entree;
    entree.type                = ENTREE_SIMPLE;
    entree.data.simple.type     = CMD_DELETE;
    entree.data.simple.position = position;
    entree.data.simple.texte    = copie;
    entree.data.simple.longueur = longueur;

    if (pousser_undo(h, &entree) != 0) {
        free(copie);
        return -1;
    }

    return 0;
}

int history_execute_replace(History* h, GapBuffer* gb,
                            size_t position,
                            const char* ancien_texte, size_t ancien_len,
                            const char* nouveau_texte, size_t nouveau_len) {
    if (!h || !gb) return -1;

    /* Copies pour l'historique */
    char* copie_ancien = NULL;
    char* copie_nouveau = NULL;

    if (ancien_len > 0 && ancien_texte) {
        copie_ancien = (char*)malloc(ancien_len + 1);
        if (!copie_ancien) return -1;
        memcpy(copie_ancien, ancien_texte, ancien_len);
        copie_ancien[ancien_len] = '\0';
    }

    if (nouveau_len > 0 && nouveau_texte) {
        copie_nouveau = (char*)malloc(nouveau_len + 1);
        if (!copie_nouveau) { free(copie_ancien); return -1; }
        memcpy(copie_nouveau, nouveau_texte, nouveau_len);
        copie_nouveau[nouveau_len] = '\0';
    }

    /* Exécution atomique via gb_replace_range */
    if (gb_replace_range(gb, position, ancien_len, nouveau_texte, nouveau_len) != 0) {
        free(copie_ancien);
        free(copie_nouveau);
        return -1;
    }

    /* Enregistrement de l'opération composite */
    EntreeHistorique entree;
    entree.type                       = ENTREE_REPLACE;
    entree.data.replace.position      = position;
    entree.data.replace.ancien_texte  = copie_ancien;
    entree.data.replace.ancien_len    = ancien_len;
    entree.data.replace.nouveau_texte = copie_nouveau;
    entree.data.replace.nouveau_len   = nouveau_len;

    if (pousser_undo(h, &entree) != 0) {
        free(copie_ancien);
        free(copie_nouveau);
        return -1;
    }

    return 0;
}

int history_undo(History* h, GapBuffer* gb) {
    if (!h || !gb || h->top_undo < 0) return -1;

    EntreeHistorique* e = &h->pile_undo[h->top_undo];

    if (e->type == ENTREE_SIMPLE) {
        Command* cmd = &e->data.simple;

        if (cmd->type == CMD_INSERT) {
            /* Annuler une insertion = supprimer ce qui a été inséré */
            gb_move_cursor(gb, cmd->position + cmd->longueur);
            gb_delete(gb, cmd->longueur);
        } else {
            /* Annuler une suppression = réinsérer le texte */
            gb_move_cursor(gb, cmd->position);
            gb_insert(gb, cmd->texte, cmd->longueur);
        }
    } else {
        /* Annuler un remplacement : remettre l'ancien texte */
        CmdReplace* r = &e->data.replace;
        gb_replace_range(gb, r->position, r->nouveau_len,
                         r->ancien_texte, r->ancien_len);
    }

    /* Déplacer de undo vers redo */
    if (h->top_redo < HISTORIQUE_MAX_COMMANDES - 1) {
        h->top_redo++;
        h->pile_redo[h->top_redo] = *e;
        /* On ne libère pas : l'entrée est maintenant dans redo */
    } else {
        /* Pile redo pleine : on libère */
        liberer_entree(e);
    }

    h->top_undo--;
    return 0;
}

int history_redo(History* h, GapBuffer* gb) {
    if (!h || !gb || h->top_redo < 0) return -1;

    EntreeHistorique* e = &h->pile_redo[h->top_redo];

    if (e->type == ENTREE_SIMPLE) {
        Command* cmd = &e->data.simple;

        if (cmd->type == CMD_INSERT) {
            /* Rétablir une insertion */
            gb_move_cursor(gb, cmd->position);
            gb_insert(gb, cmd->texte, cmd->longueur);
        } else {
            /* Rétablir une suppression */
            gb_move_cursor(gb, cmd->position + cmd->longueur);
            gb_delete(gb, cmd->longueur);
        }
    } else {
        /* Rétablir un remplacement */
        CmdReplace* r = &e->data.replace;
        gb_replace_range(gb, r->position, r->ancien_len,
                         r->nouveau_texte, r->nouveau_len);
    }

    /* Déplacer de redo vers undo */
    if (h->top_undo < HISTORIQUE_MAX_COMMANDES - 1) {
        h->top_undo++;
        h->pile_undo[h->top_undo] = *e;
    } else {
        liberer_entree(e);
    }

    h->top_redo--;
    return 0;
}