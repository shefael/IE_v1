/*
 * hunspell_wrap.c — Implémentation du wrapper Hunspell pour le français
 *
 * Hunspell expose une API C via hunspell.h (typedef Hunhandle).
 * On utilise les fonctions :
 *   Hunhandle* Hunspell_create(const char* affpath, const char* dpath)
 *   void       Hunspell_destroy(Hunhandle* pHunspell)
 *   int        Hunspell_spell(Hunhandle* pHunspell, const char* word)
 *   int        Hunspell_suggest(Hunhandle* pHunspell, char*** slst, const char* word)
 *   void       Hunspell_free_list(Hunhandle* pHunspell, char*** slst, int n)
 *   void       Hunspell_add(Hunhandle* pHunspell, const char* word)
 *
 * Note : Hunspell_spell retourne 0 si le mot est INCORRECT (logique inversée).
 */

#include "nlp/hunspell_wrap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Interface C de Hunspell (hunspell.h n'est pas toujours disponible
 * sous forme de header séparé selon la version ; on la déclare nous-mêmes
 * pour éviter les problèmes d'inclusion C++ du header officiel).
 * -------------------------------------------------------------------------- */
typedef struct Hunhandle Hunhandle;

extern Hunhandle *Hunspell_create(const char *affpath, const char *dpath);
extern void       Hunspell_destroy(Hunhandle *pHunspell);
extern int        Hunspell_spell(Hunhandle *pHunspell, const char *word);
extern int        Hunspell_suggest(Hunhandle *pHunspell, char ***slst, const char *word);
extern void       Hunspell_free_list(Hunhandle *pHunspell, char ***slst, int n);
extern int        Hunspell_add(Hunhandle *pHunspell, const char *word);

/* --------------------------------------------------------------------------
 * État interne du module (singleton)
 * -------------------------------------------------------------------------- */
static Hunhandle *g_hunspell    = NULL;
static int        g_initialise  = 0;

/* --------------------------------------------------------------------------
 * spell_init
 * -------------------------------------------------------------------------- */
int spell_init(const char *chemin_aff, const char *chemin_dic)
{
    if (g_initialise) {
        /* Déjà initialisé : on nettoie d'abord */
        Hunspell_destroy(g_hunspell);
        g_hunspell   = NULL;
        g_initialise = 0;
    }

    if (!chemin_aff || !chemin_dic) {
        fprintf(stderr, "[hunspell] Chemins AFF/DIC invalides (NULL)\n");
        return 0;
    }

    g_hunspell = Hunspell_create(chemin_aff, chemin_dic);
    if (!g_hunspell) {
        fprintf(stderr, "[hunspell] Échec de création : vérifie les chemins\n"
                        "  AFF : %s\n  DIC : %s\n", chemin_aff, chemin_dic);
        return 0;
    }

    g_initialise = 1;
    fprintf(stderr, "[hunspell] Dictionnaire français chargé avec succès\n");
    return 1;
}

/* --------------------------------------------------------------------------
 * spell_verifier
 * Retourne 1 si le mot est correct, 0 sinon.
 * Hunspell_spell retourne 0 pour INCORRECT — on inverse.
 * -------------------------------------------------------------------------- */
int spell_verifier(const char *mot)
{
    if (!g_initialise || !mot || mot[0] == '\0') {
        return 1; /* En cas de doute, on ne souligne pas */
    }

    /*
     * Pré-filtrage : on ignore les mots purement numériques,
     * les URLs, et les mots d'un seul caractère.
     */
    size_t len = strlen(mot);
    if (len <= 1) {
        return 1;
    }

    /* Ignorer si commence par http ou www */
    if (strncmp(mot, "http", 4) == 0 || strncmp(mot, "www.", 4) == 0) {
        return 1;
    }

    /* Ignorer si purement numérique (avec éventuellement . , %) */
    int est_numerique = 1;
    for (size_t i = 0; i < len; i++) {
        char c = mot[i];
        if (!((c >= '0' && c <= '9') || c == '.' || c == ',' || c == '%' || c == '-')) {
            est_numerique = 0;
            break;
        }
    }
    if (est_numerique) {
        return 1;
    }

    /* Hunspell_spell retourne 0 si INCORRECT */
    int resultat = Hunspell_spell(g_hunspell, mot);
    return (resultat != 0) ? 1 : 0;
}

/* --------------------------------------------------------------------------
 * spell_suggerer
 * -------------------------------------------------------------------------- */
char **spell_suggerer(const char *mot, int *nb_suggestions)
{
    if (!g_initialise || !mot || !nb_suggestions) {
        if (nb_suggestions) *nb_suggestions = 0;
        return NULL;
    }

    *nb_suggestions = 0;

    char **liste_hunspell = NULL;
    int nb = Hunspell_suggest(g_hunspell, &liste_hunspell, mot);

    if (nb <= 0 || !liste_hunspell) {
        return NULL;
    }

    /*
     * Hunspell_suggest retourne ses propres chaînes allouées.
     * On les copie dans notre propre tableau pour pouvoir les libérer
     * indépendamment de Hunspell, et on libère la liste Hunspell.
     */
    char **resultat = (char **)malloc((size_t)nb * sizeof(char *));
    if (!resultat) {
        Hunspell_free_list(g_hunspell, &liste_hunspell, nb);
        return NULL;
    }

    int nb_copies = 0;
    for (int i = 0; i < nb; i++) {
        if (liste_hunspell[i]) {
            resultat[nb_copies] = _strdup(liste_hunspell[i]);
            if (resultat[nb_copies]) {
                nb_copies++;
            }
        }
    }

    /* Libère la mémoire interne Hunspell */
    Hunspell_free_list(g_hunspell, &liste_hunspell, nb);

    *nb_suggestions = nb_copies;

    if (nb_copies == 0) {
        free(resultat);
        return NULL;
    }

    return resultat;
}

/* --------------------------------------------------------------------------
 * spell_liberer_suggestions
 * -------------------------------------------------------------------------- */
void spell_liberer_suggestions(char **suggestions, int nb_suggestions)
{
    if (!suggestions) return;

    for (int i = 0; i < nb_suggestions; i++) {
        free(suggestions[i]);
    }
    free(suggestions);
}

/* --------------------------------------------------------------------------
 * spell_shutdown
 * -------------------------------------------------------------------------- */
void spell_shutdown(void)
{
    if (g_initialise && g_hunspell) {
        Hunspell_destroy(g_hunspell);
        g_hunspell   = NULL;
        g_initialise = 0;
        fprintf(stderr, "[hunspell] Dictionnaire libéré\n");
    }
}

/* --------------------------------------------------------------------------
 * spell_ajouter_mot
 * -------------------------------------------------------------------------- */
void spell_ajouter_mot(const char *mot)
{
    if (!g_initialise || !mot || mot[0] == '\0') return;
    Hunspell_add(g_hunspell, mot);
}