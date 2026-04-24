/* test_nlp_temp.c — À SUPPRIMER après validation */
#include <stdio.h>
#include "nlp/hunspell_wrap.h"
#include "nlp/nlp_engine.h"

int main(void) {
    /* Initialisation Hunspell avec les dictionnaires du projet */
    if (!spell_init("data/hunspell/fr.aff", "data/hunspell/fr.dic")) {
        printf("ERREUR: Hunspell non initialisé\n");
        return 1;
    }
    nlp_init();

    const char *texte = "langague est une ortografe incorrecte du langage.";
    printf("Texte : %s\n\n", texte);

    int nb = 0;
    ErreurOrtho *erreurs = nlp_verifier_ortho(texte, &nb);

    printf("Erreurs détectées : %d\n", nb);
    for (int i = 0; i < nb; i++) {
        printf("  [%d] debut=%zu longueur=%zu\n",
               i, erreurs[i].debut, erreurs[i].longueur);
        if (erreurs[i].nb_suggestions > 0) {
            printf("       Suggestions : ");
            int max = erreurs[i].nb_suggestions < 3 ? erreurs[i].nb_suggestions : 3;
            for (int j = 0; j < max; j++) {
                printf("'%s' ", erreurs[i].suggestions[j]);
            }
            printf("\n");
        }
    }

    nlp_liberer_erreurs(erreurs, nb);
    nlp_shutdown();
    spell_shutdown();

    printf("\nOK\n");
    return 0;
}