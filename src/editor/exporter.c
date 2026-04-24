/*
 * exporter.c — Implémentation des exports TXT, RTF et .ie
 */

#include "editor/exporter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Utilitaire : écriture sécurisée
 * Retourne EXPORT_OK ou EXPORT_ERR_ECRITURE
 * -------------------------------------------------------------------------- */
static int ecrire(FILE *f, const void *donnees, size_t taille)
{
    if (taille == 0) return EXPORT_OK;
    if (fwrite(donnees, 1, taille, f) != taille) {
        return EXPORT_ERR_ECRITURE;
    }
    return EXPORT_OK;
}

/* --------------------------------------------------------------------------
 * exporter_txt
 * -------------------------------------------------------------------------- */
int exporter_txt(const char *chemin, const char *texte)
{
    if (!chemin || !texte) return EXPORT_ERR_PARAM;

    FILE *f = fopen(chemin, "wb");
    if (!f) return EXPORT_ERR_FICHIER;

    size_t len = strlen(texte);
    int ret = ecrire(f, texte, len);

    fclose(f);
    return ret;
}

/* --------------------------------------------------------------------------
 * importer_txt
 * -------------------------------------------------------------------------- */
char *importer_txt(const char *chemin)
{
    if (!chemin) return NULL;

    FILE *f = fopen(chemin, "rb");
    if (!f) return NULL;

    /* Taille du fichier */
    fseek(f, 0, SEEK_END);
    long taille = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (taille < 0) { fclose(f); return NULL; }

    char *buf = (char *)malloc((size_t)taille + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t lu = fread(buf, 1, (size_t)taille, f);
    fclose(f);

    buf[lu] = '\0';
    return buf;
}

/* --------------------------------------------------------------------------
 * RTF — Fonctions d'aide
 * -------------------------------------------------------------------------- */

/*
 * Écrit un caractère UTF-8 en RTF.
 * Les caractères ASCII s'écrivent directement sauf \, {, }.
 * Les caractères non-ASCII (multi-octets UTF-8) sont convertis
 * en séquences RTF Unicode \uN? où N est le codepoint Unicode.
 */
static int rtf_ecrire_char(FILE *f, const unsigned char *p,
                             int len_char)
{
    if (len_char == 1) {
        unsigned char c = p[0];
        /* Échapper les caractères spéciaux RTF */
        if (c == '\\') return ecrire(f, "\\\\", 2);
        if (c == '{')  return ecrire(f, "\\{",  2);
        if (c == '}')  return ecrire(f, "\\}",  2);
        if (c == '\n') return ecrire(f, "\\par\r\n", 7);
        if (c == '\r') return EXPORT_OK; /* Ignoré */
        return ecrire(f, &c, 1);
    }

    /* Décoder le codepoint UTF-8 → entier Unicode */
    uint32_t cp = 0;
    if (len_char == 2) {
        cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    } else if (len_char == 3) {
        cp = ((uint32_t)(p[0] & 0x0F) << 12) |
             ((uint32_t)(p[1] & 0x3F) << 6)  |
             (p[2] & 0x3F);
    } else if (len_char == 4) {
        cp = ((uint32_t)(p[0] & 0x07) << 18) |
             ((uint32_t)(p[1] & 0x3F) << 12) |
             ((uint32_t)(p[2] & 0x3F) << 6)  |
             (p[3] & 0x3F);
    }

    /*
     * RTF Unicode : \uN?
     * N est signé 16 bits en RTF. Pour les codepoints > 32767,
     * on soustrait 65536 pour obtenir la valeur signée.
     * Le '?' est le caractère de substitution ASCII (affiché si
     * le lecteur RTF ne supporte pas Unicode).
     */
    char buf[32];
    int32_t n = (cp > 32767) ? (int32_t)cp - 65536 : (int32_t)cp;
    int ecrit = snprintf(buf, sizeof(buf), "\\u%d?", n);
    return ecrire(f, buf, (size_t)ecrit);
}

/*
 * Retourne le style dominant à la position `pos` dans la table.
 * Priorité : TITRE > GRAS > ITALIQUE > SOULIGNE > NORMAL
 */
static StyleTexte style_a_position(const TableStyles *ts, size_t pos)
{
    if (!ts) return STYLE_NORMAL;
    return formatter_get_style(ts, pos);
}

/*
 * Émet les balises RTF d'ouverture pour un style donné.
 */
static int rtf_ouvrir_style(FILE *f, StyleTexte style)
{
    switch (style) {
        case STYLE_GRAS:       return ecrire(f, "{\\b ",    4);
        case STYLE_ITALIQUE:   return ecrire(f, "{\\i ",    4);
        case STYLE_SOULIGNE:   return ecrire(f, "{\\ul ",   5);
        case STYLE_TITRE_1:    return ecrire(f, "{\\b\\fs36 ", 10);
        case STYLE_TITRE_2:    return ecrire(f, "{\\b\\fs28 ", 10);
        case STYLE_TITRE_3:    return ecrire(f, "{\\b\\fs24 ", 10);
        case STYLE_TITRE_4:    return ecrire(f, "{\\b\\fs22 ", 10);
        case STYLE_LISTE_PUCE: return ecrire(f, "{\\pntext\\'B7\\tab}", 17);
        default:               return EXPORT_OK;
    }
}

/*
 * Émet la balise RTF de fermeture.
 */
static int rtf_fermer_style(FILE *f, StyleTexte style)
{
    if (style != STYLE_NORMAL) {
        return ecrire(f, "}", 1);
    }
    return EXPORT_OK;
}

/* --------------------------------------------------------------------------
 * exporter_rtf
 * -------------------------------------------------------------------------- */
int exporter_rtf(const char *chemin, const char *texte,
                 const TableStyles *ts)
{
    if (!chemin || !texte) return EXPORT_ERR_PARAM;

    FILE *f = fopen(chemin, "wb");
    if (!f) return EXPORT_ERR_FICHIER;

    int ret = EXPORT_OK;

    /* En-tête RTF */
    const char *entete =
        "{\\rtf1\\ansi\\ansicpg1252\\deff0\r\n"
        "{\\fonttbl{\\f0\\froman\\fcharset0 Times New Roman;}"
        "{\\f1\\fmodern\\fcharset0 Consolas;}}\r\n"
        "{\\colortbl;\\red0\\green0\\blue0;}\r\n"
        "\\f0\\fs24\\sa200\r\n";

    ret = ecrire(f, entete, strlen(entete));
    if (ret != EXPORT_OK) goto fin;

    /* Corps du texte avec styles */
    const unsigned char *p   = (const unsigned char *)texte;
    size_t               pos = 0;
    size_t               len = strlen(texte);

    StyleTexte style_courant = STYLE_NORMAL;

    while (pos < len && ret == EXPORT_OK) {
        /* Déterminer le style à cette position */
        StyleTexte style_ici = style_a_position(ts, pos);

        /* Changement de style */
        if (style_ici != style_courant) {
            /* Fermer le style précédent */
            ret = rtf_fermer_style(f, style_courant);
            if (ret != EXPORT_OK) break;
            /* Ouvrir le nouveau style */
            ret = rtf_ouvrir_style(f, style_ici);
            if (ret != EXPORT_OK) break;
            style_courant = style_ici;
        }

        /* Longueur du caractère UTF-8 courant */
        int lc = 1;
        if (p[pos] >= 0x80) {
            if ((p[pos] & 0xE0) == 0xC0)      lc = 2;
            else if ((p[pos] & 0xF0) == 0xE0)  lc = 3;
            else if ((p[pos] & 0xF8) == 0xF0)  lc = 4;
        }

        /* Écrire le caractère en RTF */
        ret = rtf_ecrire_char(f, p + pos, lc);
        pos += (size_t)lc;
    }

    /* Fermer le dernier style ouvert */
    if (ret == EXPORT_OK) {
        ret = rtf_fermer_style(f, style_courant);
    }

    /* Pied de page RTF */
    if (ret == EXPORT_OK) {
        ret = ecrire(f, "\r\n}", 3);
    }

fin:
    fclose(f);
    return ret;
}

/* --------------------------------------------------------------------------
 * exporter_ie — Format binaire propriétaire
 * -------------------------------------------------------------------------- */
int exporter_ie(const char *chemin, const char *texte,
                const TableStyles *ts)
{
    if (!chemin || !texte) return EXPORT_ERR_PARAM;

    FILE *f = fopen(chemin, "wb");
    if (!f) return EXPORT_ERR_FICHIER;

    int ret = EXPORT_OK;

    /* Magic number */
    ret = ecrire(f, "INTE", 4);
    if (ret != EXPORT_OK) goto fin;

    /* Version = 1 (uint16_t little-endian) */
    uint16_t version = 1;
    ret = ecrire(f, &version, 2);
    if (ret != EXPORT_OK) goto fin;

    /* Taille du texte (uint32_t little-endian) */
    uint32_t taille_texte = (uint32_t)strlen(texte);
    ret = ecrire(f, &taille_texte, 4);
    if (ret != EXPORT_OK) goto fin;

    /* Texte brut UTF-8 */
    ret = ecrire(f, texte, taille_texte);
    if (ret != EXPORT_OK) goto fin;

    /* Entrées de style */
    size_t            nb_entrees = 0;
    const EntreeStyle *entrees   = NULL;

    if (ts) {
        entrees = formatter_get_entrees(ts, &nb_entrees);
    }

    uint32_t nb_u32 = (uint32_t)nb_entrees;
    ret = ecrire(f, &nb_u32, 4);
    if (ret != EXPORT_OK) goto fin;

    for (size_t i = 0; i < nb_entrees && ret == EXPORT_OK; i++) {
        uint32_t offset   = (uint32_t)entrees[i].debut;
        uint32_t longueur = (uint32_t)entrees[i].longueur;
        uint8_t  style_id = (uint8_t)entrees[i].style;

        ret = ecrire(f, &offset,   4); if (ret != EXPORT_OK) break;
        ret = ecrire(f, &longueur, 4); if (ret != EXPORT_OK) break;
        ret = ecrire(f, &style_id, 1);
    }

fin:
    fclose(f);
    return ret;
}