#ifndef UTILS_ENCODING_H
#define UTILS_ENCODING_H

#ifdef __cplusplus
extern "C" {
#endif

/* Convertit une chaîne UTF-8 en UTF-16 (wchar_t sur Windows) */
wchar_t* utf8_to_utf16(const char* utf8);

/* Convertit une chaîne UTF-16 en UTF-8 */
char* utf16_to_utf8(const wchar_t* utf16);

/* Libère la mémoire d'une chaîne allouée par ces fonctions */
void encoding_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_ENCODING_H */