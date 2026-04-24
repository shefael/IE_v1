#ifndef NLP_TOKENIZER_H
#define NLP_TOKENIZER_H

/*
 * tokenizer.h — Découpage de texte UTF-8 en tokens (mots et phrases)
 *
 * IMPORTANT : tous les offsets (start, length) sont exprimés en OCTETS,
 * pas en caractères. C'est indispensable pour l'intégration avec Scintilla
 * qui travaille également en octets (SCI_INDICATORFILLRANGE).
 */

#include <stddef.h>

/* --------------------------------------------------------------------------
 * Structures
 * -------------------------------------------------------------------------- */

/* Un token = une plage dans le texte source, exprimée en octets */
typedef struct {
    size_t debut;   /* Offset en octets depuis le début du texte */
    size_t longueur; /* Longueur en octets */
    char  *texte;   /* Copie du texte du token (allouée dynamiquement) */
} Token;

typedef struct {
    Token  *items;
    size_t  nb;
} ListeTokens;

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Découpe le texte en mots.
 * Un "mot" est une séquence de caractères alphanumériques UTF-8 et
 * d'apostrophes françaises (', '). Les séparateurs sont les espaces,
 * la ponctuation, les sauts de ligne.
 *
 * Retourne une ListeTokens allouée dynamiquement.
 * Libérer avec tokenizer_liberer().
 */
ListeTokens *tokenizer_mots(const char *texte);

/*
 * Découpe le texte en phrases.
 * Une phrase se termine par '.', '!', '?', ou '…' suivi d'un espace
 * ou d'une fin de texte.
 *
 * Retourne une ListeTokens allouée dynamiquement.
 * Libérer avec tokenizer_liberer().
 */
ListeTokens *tokenizer_phrases(const char *texte);

/*
 * Libère une ListeTokens et tous ses tokens (y compris les copies texte).
 */
void tokenizer_liberer(ListeTokens *liste);

/*
 * Retourne la longueur en octets d'un caractère UTF-8
 * à partir de son premier octet. Retourne 1 en cas d'octet invalide
 * (traitement défensif).
 */
int utf8_longueur_char(unsigned char premier_octet);

#endif /* NLP_TOKENIZER_H */