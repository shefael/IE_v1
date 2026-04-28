/* Fichier de test TEMPORAIRE — supprimer après validation */
#define DEBUG
#include "include/utils/memory.h"
#include <stdio.h>

int main(void) {
    printf("=== Test 1 : allocation et libération normales ===\n");
    char* buf = (char*)ie_malloc(64);
    buf[0] = 'A'; buf[63] = 'Z';
    ie_free(buf);
    memory_rapport_fuites(); /* doit dire : aucune fuite */

    printf("\n=== Test 2 : fuite volontaire ===\n");
    char* fuite = (char*)ie_malloc(128);
    (void)fuite;
    /* on ne libère pas — mémoire_rapport_fuites doit la signaler */
    memory_rapport_fuites();
    memory_rapport_stats();

    printf("\n=== Test 3 : realloc ===\n");
    char* s = (char*)ie_malloc(10);
    s = (char*)ie_realloc(s, 200);
    ie_free(s);
    memory_rapport_fuites();

    printf("\n=== Test 4 : strdup ===\n");
    char* dup = ie_strdup("Bonjour IntelliEditor");
    printf("strdup : %s\n", dup);
    ie_free(dup);
    memory_rapport_fuites();

    return 0;
}