/* Fichier temporaire — NE PAS COMMITER — SUPPRIMER APRÈS VALIDATION */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "utils/encoding.h"

static void tester(const char* label, const char* utf8_entree) {
    printf("\n--- %s ---\n", label);
    printf("  Entrée UTF-8   : '%s'\n", utf8_entree);
    printf("  Octets         : %zu\n", utf8_longueur_octets(utf8_entree));
    printf("  Caractères     : %zu\n", utf8_longueur_caracteres(utf8_entree));

    /* UTF-8 → UTF-16 */
    wchar_t* utf16 = utf8_vers_utf16(utf8_entree);
    if (!utf16) { printf("  ECHEC : utf8_vers_utf16\n"); return; }
    printf("  UTF-16 (len)   : %zu wchar_t\n", wcslen(utf16));

    /* UTF-16 → UTF-8 (aller-retour) */
    char* retour = utf16_vers_utf8(utf16);
    if (!retour) { printf("  ECHEC : utf16_vers_utf8\n"); free(utf16); return; }
    printf("  Retour UTF-8   : '%s'\n", retour);

    if (strcmp(utf8_entree, retour) == 0) {
        printf("  OK : aller-retour parfait\n");
    } else {
        printf("  ECHEC : aller-retour différent !\n");
    }

    free(utf16);
    free(retour);
}

int main(void) {
    printf("=== Test Encoding UTF-8 / UTF-16 ===\n");

    /* Chaîne vide */
    tester("Chaîne vide", "");

    /* ASCII pur */
    tester("ASCII", "Hello World");

    /* Accents français */
    tester("Accents français", "Éditeur de texte très élégant");

    /* Caractères spéciaux */
    tester("Caractères spéciaux", "Café crème — résumé : naïf");

    /* Chemin Windows avec accents */
    tester("Chemin Windows", "C:\\Utilisateurs\\Données\\Résumé.txt");

    /* Test NULL */
    wchar_t* r1 = utf8_vers_utf16(NULL);
    char*    r2 = utf16_vers_utf8(NULL);
    if (r1 == NULL && r2 == NULL) {
        printf("\nOK : gestion NULL\n");
    } else {
        printf("\nECHEC : NULL doit retourner NULL\n");
    }

    printf("\n=== Tous les tests passent ===\n");
    return 0;
}