#ifndef UTILS_ENCODING_H
#define UTILS_ENCODING_H

#include <wchar.h>
#include <stddef.h>

/*
 * Convertit une chaîne UTF-8 en UTF-16 (wchar_t sous Windows).
 * Alloue et retourne la chaîne résultante.
 * L'appelant doit libérer la mémoire avec free().
 * Retourne NULL en cas d'erreur ou si utf8 est NULL.
 */
wchar_t* utf8_vers_utf16(const char* utf8);

/*
 * Convertit une chaîne UTF-16 (wchar_t) en UTF-8.
 * Alloue et retourne la chaîne résultante.
 * L'appelant doit libérer la mémoire avec free().
 * Retourne NULL en cas d'erreur ou si utf16 est NULL.
 */
char* utf16_vers_utf8(const wchar_t* utf16);

/*
 * Retourne la longueur en octets d'une chaîne UTF-8
 * (identique à strlen mais explicite sur l'intention).
 */
size_t utf8_longueur_octets(const char* utf8);

/*
 * Retourne le nombre de points de code Unicode dans une chaîne UTF-8.
 * (différent du nombre d'octets pour les caractères accentués)
 */
size_t utf8_longueur_caracteres(const char* utf8);

#endif /* UTILS_ENCODING_H */