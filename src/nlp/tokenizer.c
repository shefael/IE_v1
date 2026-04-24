/*
 * tokenizer.c — Tokeniseur UTF-8 avec offsets en octets
 *
 * Principe fondamental : on avance octet par octet dans le texte source,
 * en détectant les frontières de caractères UTF-8 grâce au premier octet
 * de chaque séquence. On ne coupe JAMAIS un caractère multi-octets.
 *
 * Encodage UTF-8 (rappel) :
 *   0xxxxxxx           → 1 octet  (ASCII)
 *   110xxxxx 10xxxxxx  → 2 octets (ex: é, à, ç)
 *   1110xxxx 10xxxxxx² → 3 octets (ex: €, caractères CJK)
 *   11110xxx 10xxxxxx³ → 4 octets (ex: emojis)
 *   10xxxxxx           → octet de continuation (ne commence pas un char)
 */

#include "nlp/tokenizer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * utf8_longueur_char
 * Retourne le nombre d'octets que prend le caractère UTF-8
 * dont le premier octet est `premier_octet`.
 * -------------------------------------------------------------------------- */
int utf8_longueur_char(unsigned char premier_octet)
{
    if (premier_octet < 0x80) return 1;          /* ASCII */
    if ((premier_octet & 0xE0) == 0xC0) return 2; /* 110xxxxx */
    if ((premier_octet & 0xF0) == 0xE0) return 3; /* 1110xxxx */
    if ((premier_octet & 0xF8) == 0xF0) return 4; /* 11110xxx */
    /* Octet de continuation ou invalide : on avance d'un octet */
    return 1;
}

/* --------------------------------------------------------------------------
 * Fonctions internes
 * -------------------------------------------------------------------------- */

/* Alloue une ListeTokens vide */
static ListeTokens *liste_creer(void)
{
    ListeTokens *liste = (ListeTokens *)malloc(sizeof(ListeTokens));
    if (!liste) return NULL;
    liste->items = NULL;
    liste->nb    = 0;
    return liste;
}

/* Ajoute un token à la liste (réallocation dynamique) */
static int liste_ajouter(ListeTokens *liste, size_t debut, size_t longueur,
                          const char *texte_source)
{
    if (!liste || longueur == 0) return 0;

    Token *nouveau = (Token *)realloc(liste->items,
                                       (liste->nb + 1) * sizeof(Token));
    if (!nouveau) return 0;
    liste->items = nouveau;

    Token *t = &liste->items[liste->nb];
    t->debut    = debut;
    t->longueur = longueur;

    /* Copie du texte du token */
    t->texte = (char *)malloc(longueur + 1);
    if (!t->texte) return 0;
    memcpy(t->texte, texte_source + debut, longueur);
    t->texte[longueur] = '\0';

    liste->nb++;
    return 1;
}

/*
 * Détermine si un caractère UTF-8 (dont on passe le premier octet et
 * les suivants dans `buf`) est une lettre ou un chiffre.
 *
 * Pour les caractères ASCII (1 octet) : on utilise les plages classiques.
 * Pour les caractères multi-octets UTF-8 : on considère que tout caractère
 * non-ASCII est potentiellement une lettre (accents français, etc.).
 * C'est une approximation correcte pour le français.
 */
static int est_caractere_mot(const unsigned char *p, int len_char)
{
    if (len_char == 1) {
        unsigned char c = p[0];
        /* Lettres minuscules, majuscules, chiffres, tiret interne */
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') ||
               c == '-';
    }
    /* Caractère multi-octets : on l'accepte comme lettre */
    return 1;
}

/*
 * Détermine si l'octet à la position `i` est une apostrophe
 * (ASCII ' ou Unicode ' U+2019 = E2 80 99).
 * Retourne la longueur de l'apostrophe en octets (1 ou 3), 0 sinon.
 */
static int est_apostrophe(const unsigned char *p, size_t restant)
{
    /* Apostrophe ASCII simple */
    if (p[0] == '\'') return 1;

    /* Apostrophe typographique française U+2019 : E2 80 99 */
    if (restant >= 3 &&
        p[0] == 0xE2 && p[1] == 0x80 && p[2] == 0x99) {
        return 3;
    }

    return 0;
}

/*
 * Détermine si un octet ASCII est un séparateur de phrase.
 */
static int est_fin_phrase(unsigned char c)
{
    return c == '.' || c == '!' || c == '?' || c == ';';
}

/* --------------------------------------------------------------------------
 * tokenizer_mots
 * -------------------------------------------------------------------------- */
ListeTokens *tokenizer_mots(const char *texte)
{
    if (!texte) return NULL;

    ListeTokens *liste = liste_creer();
    if (!liste) return NULL;

    const unsigned char *p   = (const unsigned char *)texte;
    size_t               pos = 0;
    size_t               len = strlen(texte);

    while (pos < len) {
        /* Sauter les séparateurs (espaces, ponctuation, sauts de ligne) */
        while (pos < len) {
            int lc = utf8_longueur_char(p[pos]);

            /* Vérifier si c'est un caractère de mot */
            if (est_caractere_mot(p + pos, lc)) break;

            /* Vérifier si c'est une apostrophe en début : on la saute */
            int apo = est_apostrophe(p + pos, len - pos);
            if (apo > 0) {
                pos += (size_t)apo;
                continue;
            }

            pos += (size_t)lc;
        }

        if (pos >= len) break;

        /* Début d'un mot trouvé */
        size_t debut_mot = pos;

        /* Avancer jusqu'à la fin du mot */
        while (pos < len) {
            int lc = utf8_longueur_char(p[pos]);

            if (est_caractere_mot(p + pos, lc)) {
                pos += (size_t)lc;
                continue;
            }

            /* Apostrophe dans un mot : l'inclure (ex: "l'école" → "l'école") */
            int apo = est_apostrophe(p + pos, len - pos);
            if (apo > 0) {
                /*
                 * On inclut l'apostrophe uniquement si le caractère
                 * suivant est une lettre (évite les apostrophes finales).
                 */
                size_t pos_apres = pos + (size_t)apo;
                if (pos_apres < len) {
                    int lc_next = utf8_longueur_char(p[pos_apres]);
                    if (est_caractere_mot(p + pos_apres, lc_next)) {
                        pos = pos_apres;
                        continue;
                    }
                }
            }

            break;
        }

        size_t longueur_mot = pos - debut_mot;
        if (longueur_mot > 0) {
            liste_ajouter(liste, debut_mot, longueur_mot, texte);
        }
    }

    return liste;
}

/* --------------------------------------------------------------------------
 * tokenizer_phrases
 * -------------------------------------------------------------------------- */
ListeTokens *tokenizer_phrases(const char *texte)
{
    if (!texte) return NULL;

    ListeTokens *liste = liste_creer();
    if (!liste) return NULL;

    const unsigned char *p   = (const unsigned char *)texte;
    size_t               pos = 0;
    size_t               len = strlen(texte);
    size_t               debut_phrase = 0;

    /* Sauter les espaces initiaux */
    while (debut_phrase < len && (p[debut_phrase] == ' ' ||
           p[debut_phrase] == '\n' || p[debut_phrase] == '\r' ||
           p[debut_phrase] == '\t')) {
        debut_phrase++;
    }
    pos = debut_phrase;

    while (pos < len) {
        int lc = utf8_longueur_char(p[pos]);

        /* Caractère ASCII : vérifier fin de phrase */
        if (lc == 1 && est_fin_phrase(p[pos])) {
            size_t fin = pos + 1;

            /* Inclure '...' ou '…' */
            while (fin < len && p[fin] == '.') fin++;

            /* La phrase va de debut_phrase à fin (inclus) */
            size_t longueur = fin - debut_phrase;
            if (longueur > 0) {
                /* Trim des espaces de fin */
                while (longueur > 0 &&
                       (texte[debut_phrase + longueur - 1] == ' '  ||
                        texte[debut_phrase + longueur - 1] == '\n' ||
                        texte[debut_phrase + longueur - 1] == '\r')) {
                    longueur--;
                }
                if (longueur > 0) {
                    liste_ajouter(liste, debut_phrase, longueur, texte);
                }
            }

            pos = fin;

            /* Sauter les espaces entre phrases */
            while (pos < len && (p[pos] == ' ' || p[pos] == '\n' ||
                   p[pos] == '\r' || p[pos] == '\t')) {
                pos++;
            }
            debut_phrase = pos;
        } else {
            pos += (size_t)lc;
        }
    }

    /* Dernière phrase sans ponctuation finale */
    if (debut_phrase < len) {
        size_t longueur = len - debut_phrase;
        /* Trim fin */
        while (longueur > 0 &&
               (texte[debut_phrase + longueur - 1] == ' '  ||
                texte[debut_phrase + longueur - 1] == '\n' ||
                texte[debut_phrase + longueur - 1] == '\r' ||
                texte[debut_phrase + longueur - 1] == '\t')) {
            longueur--;
        }
        if (longueur > 0) {
            liste_ajouter(liste, debut_phrase, longueur, texte);
        }
    }

    return liste;
}

/* --------------------------------------------------------------------------
 * tokenizer_liberer
 * -------------------------------------------------------------------------- */
void tokenizer_liberer(ListeTokens *liste)
{
    if (!liste) return;

    for (size_t i = 0; i < liste->nb; i++) {
        free(liste->items[i].texte);
    }
    free(liste->items);
    free(liste);
}