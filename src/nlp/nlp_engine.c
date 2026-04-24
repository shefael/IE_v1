/*
 * nlp_engine.c — Moteur de vérification orthographique
 *
 * Pipeline :
 *   texte brut UTF-8
 *     → tokenizer_mots()         : liste de tokens avec offsets en octets
 *     → spell_verifier(token)    : 0 = faute détectée
 *     → spell_suggerer(token)    : liste de suggestions
 *     → ErreurOrtho[]            : tableau résultat pour l'UI
 *
 * Convention critique : les offsets de ErreurOrtho sont en octets,
 * identiques à ceux attendus par SCI_INDICATORFILLRANGE de Scintilla.
 */

#include "nlp/nlp_engine.h"
#include "nlp/hunspell_wrap.h"
#include "nlp/tokenizer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * État interne
 * -------------------------------------------------------------------------- */
static int g_initialise = 0;

/* --------------------------------------------------------------------------
 * nlp_init
 * -------------------------------------------------------------------------- */
int nlp_init(void)
{
    /* Le moteur NLP ne gère pas l'initialisation de Hunspell lui-même.
     * spell_init() doit avoir été appelé avant nlp_init().
     * Ici on valide juste que le module est prêt. */
    g_initialise = 1;
    return 1;
}

/* --------------------------------------------------------------------------
 * nlp_verifier_ortho
 * -------------------------------------------------------------------------- */
ErreurOrtho *nlp_verifier_ortho(const char *texte, int *nb_erreurs)
{
    if (!nb_erreurs) return NULL;
    *nb_erreurs = 0;

    if (!g_initialise || !texte || texte[0] == '\0') {
        return NULL;
    }

    /* Étape 1 : tokeniser le texte en mots */
    ListeTokens *mots = tokenizer_mots(texte);
    if (!mots) return NULL;

    if (mots->nb == 0) {
        tokenizer_liberer(mots);
        return NULL;
    }

    /* Étape 2 : allouer le tableau de résultats (taille max = nb mots) */
    ErreurOrtho *erreurs = (ErreurOrtho *)malloc(mots->nb * sizeof(ErreurOrtho));
    if (!erreurs) {
        tokenizer_liberer(mots);
        return NULL;
    }

    int nb = 0;

    /* Étape 3 : vérifier chaque mot */
    for (size_t i = 0; i < mots->nb; i++) {
        Token *tok = &mots->items[i];

        /* On passe le texte brut du token à Hunspell */
        if (!spell_verifier(tok->texte)) {
            /* Faute détectée */
            ErreurOrtho *e = &erreurs[nb];
            e->debut    = tok->debut;
            e->longueur = tok->longueur;

            /* Récupérer les suggestions */
            e->suggestions    = spell_suggerer(tok->texte, &e->nb_suggestions);

            nb++;
        }
    }

    tokenizer_liberer(mots);

    if (nb == 0) {
        free(erreurs);
        *nb_erreurs = 0;
        return NULL;
    }

    /* Réduire l'allocation au nombre réel d'erreurs */
    ErreurOrtho *resultat = (ErreurOrtho *)realloc(erreurs,
                                                     (size_t)nb * sizeof(ErreurOrtho));
    if (!resultat) {
        /* realloc a échoué mais erreurs est toujours valide */
        resultat = erreurs;
    }

    *nb_erreurs = nb;
    return resultat;
}

/* --------------------------------------------------------------------------
 * nlp_liberer_erreurs
 * -------------------------------------------------------------------------- */
void nlp_liberer_erreurs(ErreurOrtho *erreurs, int nb_erreurs)
{
    if (!erreurs) return;

    for (int i = 0; i < nb_erreurs; i++) {
        spell_liberer_suggestions(erreurs[i].suggestions,
                                  erreurs[i].nb_suggestions);
    }
    free(erreurs);
}

/* --------------------------------------------------------------------------
 * nlp_shutdown
 * -------------------------------------------------------------------------- */
void nlp_shutdown(void)
{
    g_initialise = 0;
}