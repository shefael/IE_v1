/*
 * formatter.c — Implémentation de la table de styles
 *
 * La table est un tableau dynamique d'EntreeStyle trié par offset croissant.
 * Les plages ne se chevauchent pas : formatter_appliquer() garantit
 * l'intégrité de la table à chaque modification.
 */

#include "editor/formatter.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * Structure interne
 * -------------------------------------------------------------------------- */
struct TableStyles {
    EntreeStyle *entrees;   /* Tableau dynamique d'entrées */
    size_t       nb;        /* Nombre d'entrées actives */
    size_t       capacite;  /* Capacité allouée */
};

/* Capacité initiale et facteur de croissance */
#define CAPACITE_INITIALE 32

/* --------------------------------------------------------------------------
 * Fonctions internes
 * -------------------------------------------------------------------------- */

static int ts_agrandir(TableStyles *ts)
{
    size_t nouvelle_cap = ts->capacite == 0
                          ? CAPACITE_INITIALE
                          : ts->capacite * 2;

    EntreeStyle *nouveau = (EntreeStyle *)realloc(
        ts->entrees, nouvelle_cap * sizeof(EntreeStyle));
    if (!nouveau) return 0;

    ts->entrees  = nouveau;
    ts->capacite = nouvelle_cap;
    return 1;
}

/* Insère une entrée à l'indice idx (décale les suivantes) */
static int ts_inserer(TableStyles *ts, size_t idx, EntreeStyle e)
{
    if (ts->nb >= ts->capacite) {
        if (!ts_agrandir(ts)) return 0;
    }
    /* Décaler vers la droite */
    if (idx < ts->nb) {
        memmove(&ts->entrees[idx + 1], &ts->entrees[idx],
                (ts->nb - idx) * sizeof(EntreeStyle));
    }
    ts->entrees[idx] = e;
    ts->nb++;
    return 1;
}

/* Supprime l'entrée à l'indice idx */
static void ts_supprimer(TableStyles *ts, size_t idx)
{
    if (idx >= ts->nb) return;
    if (idx < ts->nb - 1) {
        memmove(&ts->entrees[idx], &ts->entrees[idx + 1],
                (ts->nb - idx - 1) * sizeof(EntreeStyle));
    }
    ts->nb--;
}

/* --------------------------------------------------------------------------
 * formatter_creer
 * -------------------------------------------------------------------------- */
TableStyles *formatter_creer(void)
{
    TableStyles *ts = (TableStyles *)malloc(sizeof(TableStyles));
    if (!ts) return NULL;
    ts->entrees  = NULL;
    ts->nb       = 0;
    ts->capacite = 0;
    return ts;
}

/* --------------------------------------------------------------------------
 * formatter_detruire
 * -------------------------------------------------------------------------- */
void formatter_detruire(TableStyles *ts)
{
    if (!ts) return;
    free(ts->entrees);
    free(ts);
}

/* --------------------------------------------------------------------------
 * formatter_vider
 * -------------------------------------------------------------------------- */
void formatter_vider(TableStyles *ts)
{
    if (!ts) return;
    ts->nb = 0;
}

/* --------------------------------------------------------------------------
 * formatter_appliquer
 *
 * Algorithme :
 *   1. Supprimer ou découper toutes les entrées qui chevauchent [debut, fin[
 *   2. Si style != STYLE_NORMAL, insérer la nouvelle entrée à la bonne place
 * -------------------------------------------------------------------------- */
void formatter_appliquer(TableStyles *ts, size_t debut,
                          size_t longueur, StyleTexte style)
{
    if (!ts || longueur == 0) return;

    size_t fin = debut + longueur;
    size_t i   = 0;

    while (i < ts->nb) {
        EntreeStyle *e     = &ts->entrees[i];
        size_t       e_fin = e->debut + e->longueur;

        /* Pas de chevauchement : avant la plage */
        if (e_fin <= debut) { i++; continue; }

        /* Pas de chevauchement : après la plage */
        if (e->debut >= fin) break;

        /* Chevauchement partiel à gauche : [e->debut, debut[ conservé */
        if (e->debut < debut && e_fin > debut) {
            EntreeStyle gauche = *e;
            gauche.longueur = debut - e->debut;

            /* Fragment droit éventuel : ]fin, e_fin[ */
            if (e_fin > fin) {
                EntreeStyle droite = *e;
                droite.debut    = fin;
                droite.longueur = e_fin - fin;
                /* Raccourcir l'entrée courante au fragment gauche */
                ts->entrees[i] = gauche;
                /* Insérer le fragment droit après */
                ts_inserer(ts, i + 1, droite);
                /* L'entrée courante est maintenant le fragment gauche,
                 * le suivant est le droit — on a terminé pour cette entrée */
                i += 2;
                continue;
            }

            ts->entrees[i] = gauche;
            i++;
            continue;
        }

        /* Chevauchement partiel à droite : ]fin, e_fin[ conservé */
        if (e->debut < fin && e_fin > fin) {
            ts->entrees[i].debut    = fin;
            ts->entrees[i].longueur = e_fin - fin;
            break; /* Cette entrée commence après notre plage, on arrête */
        }

        /* Chevauchement total : supprimer l'entrée */
        ts_supprimer(ts, i);
        /* Ne pas incrémenter i : la suppression a décalé les suivantes */
    }

    /* Insérer la nouvelle entrée si style != NORMAL */
    if (style == STYLE_NORMAL) return;

    /* Trouver la position d'insertion (ordre croissant par debut) */
    size_t pos_insert = 0;
    while (pos_insert < ts->nb && ts->entrees[pos_insert].debut < debut) {
        pos_insert++;
    }

    EntreeStyle nouvelle = { debut, longueur, style };
    ts_inserer(ts, pos_insert, nouvelle);
}

/* --------------------------------------------------------------------------
 * formatter_get_style
 * -------------------------------------------------------------------------- */
StyleTexte formatter_get_style(const TableStyles *ts, size_t position)
{
    if (!ts) return STYLE_NORMAL;

    for (size_t i = 0; i < ts->nb; i++) {
        const EntreeStyle *e = &ts->entrees[i];
        if (position >= e->debut && position < e->debut + e->longueur) {
            return e->style;
        }
        /* Tableau trié : si l'entrée commence après position, inutile
         * de continuer */
        if (e->debut > position) break;
    }
    return STYLE_NORMAL;
}

/* --------------------------------------------------------------------------
 * formatter_get_entrees
 * -------------------------------------------------------------------------- */
const EntreeStyle *formatter_get_entrees(const TableStyles *ts,
                                          size_t *nb_entrees)
{
    if (!ts || !nb_entrees) {
        if (nb_entrees) *nb_entrees = 0;
        return NULL;
    }
    *nb_entrees = ts->nb;
    return ts->entrees;
}

/* --------------------------------------------------------------------------
 * formatter_decaler
 *
 * Après une insertion de `delta` octets à `position` :
 *   - Les entrées dont le début est >= position sont décalées de delta.
 *   - Les entrées qui chevauchent position sont étendues (insertion)
 *     ou tronquées (suppression, delta < 0).
 * -------------------------------------------------------------------------- */
void formatter_decaler(TableStyles *ts, size_t position, int delta)
{
    if (!ts || delta == 0) return;

    for (size_t i = 0; i < ts->nb; i++) {
        EntreeStyle *e     = &ts->entrees[i];
        size_t       e_fin = e->debut + e->longueur;

        if (delta > 0) {
            /* Insertion */
            if (e->debut >= position) {
                e->debut += (size_t)delta;
            } else if (e_fin > position) {
                /* L'entrée chevauche le point d'insertion : on l'étend */
                e->longueur += (size_t)delta;
            }
        } else {
            /* Suppression */
            size_t suppression = (size_t)(-delta);
            size_t fin_suppr   = position + suppression;

            if (e->debut >= fin_suppr) {
                /* Entrée entièrement après la zone supprimée */
                e->debut -= suppression;
            } else if (e_fin <= position) {
                /* Entrée entièrement avant : inchangée */
            } else {
                /* Chevauchement : recalcul */
                size_t nouveau_debut = e->debut < position
                                       ? e->debut : position;
                size_t nouveau_fin   = e_fin > fin_suppr
                                       ? e_fin - suppression : position;
                if (nouveau_fin <= nouveau_debut) {
                    /* L'entrée est entièrement supprimée */
                    ts_supprimer(ts, i);
                    i--;
                } else {
                    e->debut    = nouveau_debut;
                    e->longueur = nouveau_fin - nouveau_debut;
                }
            }
        }
    }
}