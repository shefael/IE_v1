#include "utils/encoding.h"

#include <stdlib.h>
#include <string.h>

/*
 * Sous Windows, wchar_t est de 2 octets (UTF-16LE).
 * On utilise les fonctions WinAPI :
 *   MultiByteToWideChar  : UTF-8 → UTF-16
 *   WideCharToMultiByte  : UTF-16 → UTF-8
 *
 * CP_UTF8 = 65001 (page de codes UTF-8 de Windows)
 */
#include <windows.h>

wchar_t* utf8_vers_utf16(const char* utf8) {
    if (!utf8) return NULL;

    /* Premier appel : obtenir la taille nécessaire (en wchar_t, terminateur inclus) */
    int taille = MultiByteToWideChar(
        CP_UTF8,          /* page de codes source              */
        0,                /* pas de flags spéciaux             */
        utf8,             /* chaîne source                     */
        -1,               /* -1 = chaîne terminée par '\0'     */
        NULL,             /* buffer de sortie NULL = calcul    */
        0                 /* taille 0 = mode calcul            */
    );

    if (taille <= 0) return NULL;

    wchar_t* resultat = (wchar_t*)malloc(sizeof(wchar_t) * (size_t)taille);
    if (!resultat) return NULL;

    /* Deuxième appel : conversion effective */
    int ret = MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8,
        -1,
        resultat,
        taille
    );

    if (ret <= 0) {
        free(resultat);
        return NULL;
    }

    return resultat;
}

char* utf16_vers_utf8(const wchar_t* utf16) {
    if (!utf16) return NULL;

    /* Premier appel : calculer la taille nécessaire en octets */
    int taille = WideCharToMultiByte(
        CP_UTF8,    /* page de codes cible                   */
        0,          /* pas de flags                          */
        utf16,      /* chaîne source UTF-16                  */
        -1,         /* terminée par '\0'                     */
        NULL,       /* buffer NULL = mode calcul             */
        0,          /* taille 0 = mode calcul                */
        NULL,       /* caractère par défaut (NULL = aucun)   */
        NULL        /* indicateur de caractère par défaut    */
    );

    if (taille <= 0) return NULL;

    char* resultat = (char*)malloc((size_t)taille);
    if (!resultat) return NULL;

    /* Deuxième appel : conversion effective */
    int ret = WideCharToMultiByte(
        CP_UTF8,
        0,
        utf16,
        -1,
        resultat,
        taille,
        NULL,
        NULL
    );

    if (ret <= 0) {
        free(resultat);
        return NULL;
    }

    return resultat;
}

size_t utf8_longueur_octets(const char* utf8) {
    if (!utf8) return 0;
    return strlen(utf8);
}

size_t utf8_longueur_caracteres(const char* utf8) {
    if (!utf8) return 0;

    size_t compte = 0;
    const unsigned char* p = (const unsigned char*)utf8;

    /*
     * En UTF-8, un octet de continuation commence par 10xxxxxx (0x80..0xBF).
     * On compte tous les octets qui NE sont PAS des octets de continuation,
     * ce qui donne le nombre de points de code.
     */
    while (*p) {
        if ((*p & 0xC0) != 0x80) {
            compte++;
        }
        p++;
    }

    return compte;
}