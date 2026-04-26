/*
 * search_replace.c — Recherche et remplacement avec support PCRE2
 *
 * PCRE2 est utilisé pour les regex. Pour la recherche simple,
 * on utilise une implémentation maison UTF-8 aware basée sur strstr
 * avec gestion de la casse via strlwr temporaire.
 *
 * Intégration PCRE2 :
 *   On définit PCRE2_CODE_UNIT_WIDTH 8 avant d'inclure pcre2.h
 *   pour travailler en UTF-8 (unités de 8 bits).
 */

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "editor/search_replace.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
 * Structure interne
 * -------------------------------------------------------------------------- */
struct ContexteRecherche {
    char  *requete;          /* Copie de la requête originale */
    char  *requete_min;      /* Requête en minuscules (mode insensible) */
    int    utiliser_regex;   /* 1 = PCRE2, 0 = texte simple */
    int    sensible_casse;   /* 1 = respecte la casse */

    /* Données PCRE2 (NULL si mode texte simple) */
    pcre2_code            *re;
    pcre2_match_data      *match_data;
};

/* --------------------------------------------------------------------------
 * Utilitaire : copie en minuscules ASCII
 * Pour le français on ne descend pas en dessous de 0x80,
 * on utilise tolower() uniquement sur les octets ASCII.
 * -------------------------------------------------------------------------- */
static char *dupliquer_minuscules(const char *src)
{
    if (!src) return NULL;
    size_t len = strlen(src);
    char  *dst = (char *)malloc(len + 1);
    if (!dst) return NULL;
    for (size_t i = 0; i <= len; i++) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }
    return dst;
}

/* --------------------------------------------------------------------------
 * recherche_creer
 * -------------------------------------------------------------------------- */
ContexteRecherche *recherche_creer(const char *requete,
                                    int utiliser_regex,
                                    int sensible_casse)
{
    if (!requete || requete[0] == '\0') return NULL;

    ContexteRecherche *ctx = (ContexteRecherche *)calloc(
        1, sizeof(ContexteRecherche));
    if (!ctx) return NULL;

    ctx->requete       = _strdup(requete);
    ctx->requete_min   = dupliquer_minuscules(requete);
    ctx->utiliser_regex = utiliser_regex;
    ctx->sensible_casse = sensible_casse;
    ctx->re             = NULL;
    ctx->match_data     = NULL;

    if (!ctx->requete || !ctx->requete_min) {
        recherche_detruire(ctx);
        return NULL;
    }

    if (utiliser_regex) {
        /* Compilation de la regex PCRE2 */
        int        errcode;
        PCRE2_SIZE erroffset;
        uint32_t   options = PCRE2_UTF;

        if (!sensible_casse) {
            options |= PCRE2_CASELESS;
        }

        ctx->re = pcre2_compile(
            (PCRE2_SPTR)requete,
            PCRE2_ZERO_TERMINATED,
            options,
            &errcode,
            &erroffset,
            NULL
        );

        if (!ctx->re) {
            /* Regex invalide : afficher le message d'erreur */
            PCRE2_UCHAR tampon_err[256];
            pcre2_get_error_message(errcode, tampon_err, sizeof(tampon_err));
            fprintf(stderr, "[pcre2] Erreur de compilation à l'offset %zu : %s\n",
                    erroffset, (char *)tampon_err);
            recherche_detruire(ctx);
            return NULL;
        }

        ctx->match_data = pcre2_match_data_create_from_pattern(ctx->re, NULL);
        if (!ctx->match_data) {
            recherche_detruire(ctx);
            return NULL;
        }
    }

    return ctx;
}

/* --------------------------------------------------------------------------
 * recherche_detruire
 * -------------------------------------------------------------------------- */
void recherche_detruire(ContexteRecherche *ctx)
{
    if (!ctx) return;
    free(ctx->requete);
    free(ctx->requete_min);
    if (ctx->match_data) pcre2_match_data_free(ctx->match_data);
    if (ctx->re)         pcre2_code_free(ctx->re);
    free(ctx);
}

/* --------------------------------------------------------------------------
 * Recherche simple (sans regex) — UTF-8 aware
 *
 * On compare octet par octet. Pour la recherche insensible à la casse,
 * on convertit le texte source en minuscules dans un buffer temporaire.
 * -------------------------------------------------------------------------- */
static int recherche_simple(ContexteRecherche *ctx,
                              const char *texte,
                              size_t position_depart,
                              size_t *out_debut,
                              size_t *out_longueur)
{
    size_t len_texte   = strlen(texte);
    size_t len_requete = strlen(ctx->requete);

    if (position_depart >= len_texte || len_requete == 0) return 0;

    const char *requete_cherchee = ctx->sensible_casse
                                   ? ctx->requete
                                   : ctx->requete_min;

    /* Buffer temporaire en minuscules si insensible à la casse */
    char *texte_min = NULL;
    const char *texte_cherche = texte;

    if (!ctx->sensible_casse) {
        texte_min = dupliquer_minuscules(texte);
        if (!texte_min) return 0;
        texte_cherche = texte_min;
    }

    /* Recherche naïve à partir de position_depart */
    const char *debut_recherche = texte_cherche + position_depart;
    const char *trouve = strstr(debut_recherche, requete_cherchee);

    int resultat = 0;
    if (trouve) {
        *out_debut    = (size_t)(trouve - texte_cherche);
        *out_longueur = len_requete;
        resultat = 1;
    }

    free(texte_min);
    return resultat;
}

/* --------------------------------------------------------------------------
 * recherche_suivante
 * -------------------------------------------------------------------------- */
int recherche_suivante(ContexteRecherche *ctx,
                        const char *texte,
                        size_t position_depart,
                        size_t *out_debut,
                        size_t *out_longueur)
{
    if (!ctx || !texte || !out_debut || !out_longueur) return 0;

    *out_debut    = 0;
    *out_longueur = 0;

    if (!ctx->utiliser_regex) {
        return recherche_simple(ctx, texte, position_depart,
                                out_debut, out_longueur);
    }

    /* Mode PCRE2 */
    size_t len_texte = strlen(texte);
    if (position_depart >= len_texte) return 0;

    int rc = pcre2_match(
        ctx->re,
        (PCRE2_SPTR)texte,
        len_texte,
        position_depart,
        0,
        ctx->match_data,
        NULL
    );

    if (rc < 0) {
        /* Pas de correspondance ou erreur */
        if (rc != PCRE2_ERROR_NOMATCH) {
            fprintf(stderr, "[pcre2] Erreur de matching : %d\n", rc);
        }
        return 0;
    }

    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(ctx->match_data);
    *out_debut    = (size_t)ovector[0];
    *out_longueur = (size_t)(ovector[1] - ovector[0]);
    return 1;
}

/* --------------------------------------------------------------------------
 * recherche_remplacer_tout
 * -------------------------------------------------------------------------- */
char *recherche_remplacer_tout(ContexteRecherche *ctx,
                                const char *texte,
                                const char *remplacement,
                                int *nb_remplacements)
{
    if (!ctx || !texte || !remplacement) {
        if (nb_remplacements) *nb_remplacements = 0;
        return NULL;
    }
    if (nb_remplacements) *nb_remplacements = 0;

    size_t len_texte       = strlen(texte);
    size_t len_remplacement = strlen(remplacement);

    /*
     * Stratégie : on construit le résultat dans un buffer dynamique.
     * On parcourt le texte, on cherche chaque occurrence, et on copie
     * les segments entre les occurrences + le remplacement.
     */
    size_t capacite = len_texte + 1024; /* Taille initiale généreuse */
    char  *resultat = (char *)malloc(capacite);
    if (!resultat) return NULL;

    size_t pos_lecture  = 0; /* Position dans le texte source */
    size_t pos_ecriture = 0; /* Position dans le buffer résultat */
    int    nb           = 0;

    while (pos_lecture <= len_texte) {
        size_t debut     = 0;
        size_t longueur  = 0;

        int trouve = recherche_suivante(ctx, texte, pos_lecture,
                                        &debut, &longueur);

        if (!trouve || longueur == 0) {
            /* Copier le reste du texte */
            size_t reste = len_texte - pos_lecture;
            size_t besoin = pos_ecriture + reste + 1;

            if (besoin > capacite) {
                capacite = besoin * 2;
                char *tmp = (char *)realloc(resultat, capacite);
                if (!tmp) { free(resultat); return NULL; }
                resultat = tmp;
            }

            memcpy(resultat + pos_ecriture, texte + pos_lecture, reste);
            pos_ecriture += reste;
            break;
        }

        /* Copier le segment avant l'occurrence */
        size_t segment = debut - pos_lecture;
        size_t besoin  = pos_ecriture + segment + len_remplacement + 1;

        if (besoin > capacite) {
            capacite = besoin * 2;
            char *tmp = (char *)realloc(resultat, capacite);
            if (!tmp) { free(resultat); return NULL; }
            resultat = tmp;
        }

        memcpy(resultat + pos_ecriture, texte + pos_lecture, segment);
        pos_ecriture += segment;

        /* Écrire le remplacement */
        memcpy(resultat + pos_ecriture, remplacement, len_remplacement);
        pos_ecriture += len_remplacement;

        /* Avancer après l'occurrence */
        pos_lecture = debut + longueur;
        nb++;

        /* Sécurité : éviter boucle infinie sur match de longueur 0 */
        if (longueur == 0) {
            if (pos_lecture < len_texte) {
                resultat[pos_ecriture++] = texte[pos_lecture++];
            } else {
                break;
            }
        }
    }

    resultat[pos_ecriture] = '\0';
    if (nb_remplacements) *nb_remplacements = nb;
    return resultat;
}

/* --------------------------------------------------------------------------
 * recherche_get_requete
 * -------------------------------------------------------------------------- */
const char *recherche_get_requete(const ContexteRecherche *ctx)
{
    if (!ctx) return NULL;
    return ctx->requete;
}