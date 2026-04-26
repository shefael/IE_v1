#ifndef EDITOR_SEARCH_REPLACE_H
#define EDITOR_SEARCH_REPLACE_H

/*
 * search_replace.h — Recherche et remplacement dans le texte
 *
 * Supporte deux modes :
 *   - Recherche simple (strstr UTF-8 aware)
 *   - Recherche par expression régulière via PCRE2
 *
 * Les offsets retournés sont toujours en OCTETS (compatibles Scintilla).
 */

#include <stddef.h>

/* --------------------------------------------------------------------------
 * Structure de contexte de recherche (opaque)
 * -------------------------------------------------------------------------- */
typedef struct ContexteRecherche ContexteRecherche;

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Crée un contexte de recherche.
 * requete         : chaîne à rechercher (UTF-8)
 * utiliser_regex  : 1 = mode regex PCRE2, 0 = mode texte simple
 * sensible_casse  : 1 = respect de la casse, 0 = insensible
 *
 * Retourne NULL en cas d'erreur (regex invalide, mémoire insuffisante).
 * En cas d'erreur de regex, le message est écrit dans stderr.
 */
ContexteRecherche *recherche_creer(const char *requete,
                                    int utiliser_regex,
                                    int sensible_casse);

/*
 * Libère un contexte de recherche.
 */
void recherche_detruire(ContexteRecherche *ctx);

/*
 * Trouve la prochaine occurrence dans `texte` à partir de `position_depart`.
 * Remplit *out_debut et *out_longueur avec la position et la taille
 * de la correspondance (en octets).
 *
 * Retourne 1 si une occurrence est trouvée, 0 sinon.
 */
int recherche_suivante(ContexteRecherche *ctx,
                        const char *texte,
                        size_t position_depart,
                        size_t *out_debut,
                        size_t *out_longueur);

/*
 * Remplace toutes les occurrences de la requête dans `texte` par
 * `remplacement`.
 *
 * Retourne le nouveau texte alloué dynamiquement (libérer avec free()).
 * Retourne NULL en cas d'erreur.
 * *nb_remplacements est mis à jour avec le nombre de remplacements effectués.
 */
char *recherche_remplacer_tout(ContexteRecherche *ctx,
                                const char *texte,
                                const char *remplacement,
                                int *nb_remplacements);

/*
 * Retourne la requête originale du contexte.
 */
const char *recherche_get_requete(const ContexteRecherche *ctx);

#endif /* EDITOR_SEARCH_REPLACE_H */