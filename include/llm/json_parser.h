#ifndef LLM_JSON_PARSER_H
#define LLM_JSON_PARSER_H

/*
 * json_parser.h — Parser des réponses JSON du LLM
 *
 * Le LLM peut parfois retourner du texte libre au lieu du JSON attendu.
 * Ce module gère les erreurs de parsing gracieusement.
 */

#include <stddef.h>

/* --------------------------------------------------------------------------
 * Structures de résultat
 * -------------------------------------------------------------------------- */

/* Une erreur grammaticale détectée par le LLM */
typedef struct {
    char *type;        /* subject_verb_agreement, etc. */
    char *original;    /* Texte fautif */
    char *corrected;   /* Texte corrigé */
    char *explanation; /* Explication */
} ErreurGrammaire;

/* Résultat d'une reformulation */
typedef struct {
    char *reformule;   /* Texte reformulé */
    char *explication; /* Explication des changements */
} ResultatReformulation;

/* Résultat d'une vérification sémantique */
typedef struct {
    int    reponse;     /* 1 = oui, 0 = non */
    double confiance;   /* 0.0 à 1.0 */
    char  *justification;
} ResultatSemantique;

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Parse une réponse de correction grammaticale.
 * Retourne un tableau d'ErreurGrammaire alloué dynamiquement.
 * *nb_erreurs est mis à jour.
 * Retourne NULL si le JSON est invalide ou s'il n'y a pas d'erreurs.
 * Libérer avec json_parser_liberer_erreurs().
 */
ErreurGrammaire *json_parser_erreurs_grammaire(const char *json_brut,
                                                int *nb_erreurs);

/*
 * Parse une réponse de reformulation.
 * Retourne un ResultatReformulation alloué dynamiquement.
 * Retourne NULL si le JSON est invalide.
 * Libérer avec json_parser_liberer_reformulation().
 */
ResultatReformulation *json_parser_reformulation(const char *json_brut);

/*
 * Parse une réponse de vérification sémantique.
 * Retourne un ResultatSemantique alloué dynamiquement.
 * Retourne NULL si le JSON est invalide.
 * Libérer avec json_parser_liberer_semantique().
 */
ResultatSemantique *json_parser_semantique(const char *json_brut);

/*
 * Fonctions de libération mémoire.
 */
void json_parser_liberer_erreurs(ErreurGrammaire *erreurs, int nb);
void json_parser_liberer_reformulation(ResultatReformulation *res);
void json_parser_liberer_semantique(ResultatSemantique *res);

/*
 * Tente d'extraire un bloc JSON valide depuis une chaîne qui peut
 * contenir du texte libre avant/après le JSON.
 * Retourne un buffer alloué dynamiquement (libérer avec free()).
 * Retourne NULL si aucun JSON valide n'est trouvé.
 */
char *json_parser_extraire_json(const char *texte_brut);

#endif /* LLM_JSON_PARSER_H */