/* test_tok_temp.c — À SUPPRIMER après validation */
#include <stdio.h>
#include "nlp/tokenizer.h"

int main(void) {
    const char *texte = "L'\xc3\xa9" "diteur est pr\xc3\xaat. Bonjour le monde!";
    /* = "L'éditeur est prêt. Bonjour le monde!" en UTF-8 */

    printf("=== Test tokenizer_mots ===\n");
    ListeTokens *mots = tokenizer_mots(texte);
    if (!mots) { printf("ERREUR: NULL\n"); return 1; }
    for (size_t i = 0; i < mots->nb; i++) {
        printf("  [%zu] debut=%zu len=%zu texte='%s'\n",
               i, mots->items[i].debut,
               mots->items[i].longueur,
               mots->items[i].texte);
    }
    tokenizer_liberer(mots);

    printf("\n=== Test tokenizer_phrases ===\n");
    ListeTokens *phrases = tokenizer_phrases(texte);
    if (!phrases) { printf("ERREUR: NULL\n"); return 1; }
    for (size_t i = 0; i < phrases->nb; i++) {
        printf("  [%zu] debut=%zu len=%zu texte='%s'\n",
               i, phrases->items[i].debut,
               phrases->items[i].longueur,
               phrases->items[i].texte);
    }
    tokenizer_liberer(phrases);

    printf("\nOK\n");
    return 0;
}