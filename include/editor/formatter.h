#ifndef EDITOR_FORMATTER_H
#define EDITOR_FORMATTER_H

/*
 * formatter.h — Styles inline pour le gap buffer
 *
 * Le formatter maintient une table de styles parallèle au gap buffer.
 * Chaque entrée associe une plage d'octets (offset + longueur) à un style.
 * Cette table est utilisée par l'exporter pour générer le RTF.
 */

#include <stddef.h>

/* --------------------------------------------------------------------------
 * Enumération des styles supportés
 * -------------------------------------------------------------------------- */
typedef enum {
    STYLE_NORMAL        = 0,
    STYLE_GRAS          = 1,
    STYLE_ITALIQUE      = 2,
    STYLE_SOULIGNE      = 3,
    STYLE_TITRE_1       = 4,
    STYLE_TITRE_2       = 5,
    STYLE_TITRE_3       = 6,
    STYLE_TITRE_4       = 7,
    STYLE_LISTE_PUCE    = 8,
    STYLE_LISTE_NUM     = 9
} StyleTexte;

/* --------------------------------------------------------------------------
 * Une entrée dans la table des styles
 * -------------------------------------------------------------------------- */
typedef struct {
    size_t     debut;    /* Offset en octets depuis le début du texte */
    size_t     longueur; /* Longueur en octets de la plage stylisée */
    StyleTexte style;    /* Style appliqué */
} EntreeStyle;

/* --------------------------------------------------------------------------
 * Table des styles (structure opaque)
 * -------------------------------------------------------------------------- */
typedef struct TableStyles TableStyles;

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/* Crée une table de styles vide. */
TableStyles *formatter_creer(void);

/* Détruit la table et libère la mémoire. */
void formatter_detruire(TableStyles *ts);

/* Vide la table (supprime toutes les entrées). */
void formatter_vider(TableStyles *ts);

/*
 * Applique un style sur une plage [debut, debut+longueur[.
 * Si la plage chevauche des entrées existantes, elles sont fusionnées
 * ou découpées selon les besoins.
 * STYLE_NORMAL supprime tout style sur la plage.
 */
void formatter_appliquer(TableStyles *ts, size_t debut,
                          size_t longueur, StyleTexte style);

/*
 * Retourne le style dominant à la position donnée.
 * Retourne STYLE_NORMAL si aucun style n'est défini.
 */
StyleTexte formatter_get_style(const TableStyles *ts, size_t position);

/*
 * Retourne un pointeur vers le tableau interne des entrées.
 * *nb_entrees est mis à jour avec le nombre d'entrées actives.
 * Ne pas libérer le tableau retourné.
 */
const EntreeStyle *formatter_get_entrees(const TableStyles *ts,
                                          size_t *nb_entrees);

/*
 * Décale tous les offsets >= position de `delta` octets.
 * Appelé après une insertion dans le gap buffer pour maintenir
 * la cohérence des offsets.
 * delta peut être négatif (suppression).
 */
void formatter_decaler(TableStyles *ts, size_t position, int delta);

#endif /* EDITOR_FORMATTER_H */