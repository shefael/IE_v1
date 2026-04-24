#ifndef NLP_NLP_ENGINE_H
#define NLP_NLP_ENGINE_H

/*
 * nlp_engine.h — Moteur de vérification orthographique
 *
 * Orchestre le tokeniseur et Hunspell pour produire une liste
 * d'erreurs orthographiques avec leurs positions en octets UTF-8.
 *
 * Les offsets sont en OCTETS (compatibles Scintilla SCI_INDICATORFILLRANGE).
 */

#include <stddef.h>

/* --------------------------------------------------------------------------
 * Structures
 * -------------------------------------------------------------------------- */

/*
 * Une erreur orthographique détectée dans le texte.
 * start et longueur sont en octets UTF-8.
 */
typedef struct {
    size_t  debut;          /* Offset en octets depuis le début du texte */
    size_t  longueur;       /* Longueur en octets du mot fautif */
    char  **suggestions;    /* Tableau de suggestions (peut être NULL) */
    int     nb_suggestions; /* Nombre de suggestions */
} ErreurOrtho;

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Initialise le moteur NLP.
 * Doit être appelé après spell_init().
 * Retourne 1 en cas de succès, 0 sinon.
 */
int nlp_init(void);

/*
 * Vérifie l'orthographe du texte complet.
 * Retourne un tableau d'ErreurOrtho alloué dynamiquement.
 * *nb_erreurs est mis à jour avec le nombre d'erreurs trouvées.
 * Retourne NULL si aucune erreur ou en cas d'erreur d'allocation.
 * Libérer avec nlp_liberer_erreurs().
 */
ErreurOrtho *nlp_verifier_ortho(const char *texte, int *nb_erreurs);

/*
 * Libère un tableau d'ErreurOrtho.
 */
void nlp_liberer_erreurs(ErreurOrtho *erreurs, int nb_erreurs);

/*
 * Libère les ressources du moteur NLP.
 */
void nlp_shutdown(void);

#endif /* NLP_NLP_ENGINE_H */