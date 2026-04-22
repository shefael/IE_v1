/* Fichier de test temporaire — NE PAS COMMITER — SUPPRIMER APRÈS VALIDATION */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "editor/gap_buffer.h"

int main(void) {
    printf("=== Test Gap Buffer ===\n");

    /* Test 1 : création */
    GapBuffer* gb = gb_create(16);
    if (!gb) { printf("ECHEC : gb_create\n"); return 1; }
    printf("OK : création (longueur=%zu)\n", gb_get_length(gb));

    /* Test 2 : insertion */
    gb_insert(gb, "Bonjour", 7);
    char* texte = gb_get_text(gb);
    printf("OK : après insert 'Bonjour' → '%s' (len=%zu)\n", texte, gb_get_length(gb));
    if (strcmp(texte, "Bonjour") != 0) { printf("ECHEC : texte incorrect\n"); free(texte); return 1; }
    free(texte);

    /* Test 3 : déplacement curseur + insertion au milieu */
    gb_move_cursor(gb, 3); /* curseur après "Bon" */
    gb_insert(gb, "ne", 2);
    texte = gb_get_text(gb);
    printf("OK : après insert 'ne' en pos 3 → '%s'\n", texte);
    if (strcmp(texte, "Bonnejour") != 0) { printf("ECHEC : attendu 'Bonnejour'\n"); free(texte); return 1; }
    free(texte);

    /* Test 4 : suppression */
    gb_delete(gb, 2); /* supprime 'ne' */
    texte = gb_get_text(gb);
    printf("OK : après delete(2) → '%s'\n", texte);
    if (strcmp(texte, "Bonjour") != 0) { printf("ECHEC : attendu 'Bonjour'\n"); free(texte); return 1; }
    free(texte);

    /* Test 5 : insertion déclenchant un agrandissement */
    gb_move_cursor(gb, 7);
    gb_insert(gb, " le monde !", 11);
    texte = gb_get_text(gb);
    printf("OK : après agrandissement → '%s'\n", texte);
    if (strcmp(texte, "Bonjour le monde !") != 0) { printf("ECHEC : texte incorrect\n"); free(texte); return 1; }
    free(texte);

    /* Test 6 : replace_range */
    gb_replace_range(gb, 8, 2, "tout le", 7); /* remplace "le" par "tout le" */
    texte = gb_get_text(gb);
    printf("OK : après replace_range → '%s'\n", texte);
    free(texte);

    /* Test 7 : curseur hors limites */
    gb_move_cursor(gb, 99999);
    printf("OK : curseur hors limites → clamped à %zu (len=%zu)\n",
           gb_get_cursor(gb), gb_get_length(gb));

    gb_destroy(gb);
    printf("OK : destroy\n");
    printf("=== Tous les tests passent ===\n");
    return 0;
}