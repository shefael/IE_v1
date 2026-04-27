/*
 * count_checker.c — Vérificateurs de nombre de mots par section
 * Projet : IntelliEditor
 */

#include "rules/checkers/count_checker.h"
#include "rules/rule_engine.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
 * Utilitaires internes
 * -------------------------------------------------------------------------- */

static RuleResult faire_resultat(const char *rule_id,
                                 RuleStatus statut,
                                 const char *message)
{
    RuleResult res;
    
    // Correction : Utilisation de strncpy pour les tableaux fixes
    if (rule_id) {
        strncpy(res.rule_id, rule_id, sizeof(res.rule_id) - 1);
        res.rule_id[sizeof(res.rule_id) - 1] = '\0';
    } else {
        strncpy(res.rule_id, "?", sizeof(res.rule_id) - 1);
    }

    // Correction : 'statut' au lieu de 'status'
    res.statut = statut;

    // Correction : Utilisation de strncpy pour le message
    if (message) {
        strncpy(res.message, message, sizeof(res.message) - 1);
        res.message[sizeof(res.message) - 1] = '\0';
    } else {
        res.message[0] = '\0';
    }

    return res;
}

static char *minuscules(const char *src)
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

static void trim(char *s)
{
    if (!s) return;
    size_t debut = 0;
    while (s[debut] && isspace((unsigned char)s[debut])) debut++;
    if (debut > 0) memmove(s, s + debut, strlen(s) - debut + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

static size_t compter_mots(const char *texte)
{
    if (!texte) return 0;
    size_t compte   = 0;
    int    dans_mot = 0;
    const unsigned char *p = (const unsigned char *)texte;

    while (*p) {
        int est_separateur = (*p == ' ' || *p == '\t' ||
                              *p == '\n' || *p == '\r');

        if (!est_separateur && !dans_mot) {
            compte++;
            dans_mot = 1;
        } else if (est_separateur) {
            dans_mot = 0;
        }
        p++;
    }
    return compte;
}

static char *extraire_section(const char *texte, const char *titre)
{
    if (!texte || !titre) return NULL;

    char *texte_min = minuscules(texte);
    char *titre_min = minuscules(titre);
    if (!texte_min || !titre_min) {
        free(texte_min);
        free(titre_min);
        return NULL;
    }

    trim(titre_min);
    size_t len_titre  = strlen(titre_min);
    size_t len_texte  = strlen(texte);
    size_t pos        = 0;
    long   debut_contenu = -1;

    while (pos < len_texte) {
        size_t debut_ligne = pos;
        size_t fin_ligne   = pos;
        while (fin_ligne < len_texte && texte_min[fin_ligne] != '\n') {
            fin_ligne++;
        }

        size_t len_ligne = fin_ligne - debut_ligne;
        if (len_ligne > 0 && len_ligne < 256) {
            char ligne[256];
            memcpy(ligne, texte_min + debut_ligne, len_ligne);
            ligne[len_ligne] = '\0';
            trim(ligne);

            if (strlen(ligne) > 0 && strstr(ligne, titre_min) != NULL) {
                if (len_titre * 2 >= strlen(ligne)) {
                    debut_contenu = (long)(fin_ligne + 1);
                    break;
                }
            }
        }
        pos = fin_ligne + 1;
    }

    free(texte_min);
    free(titre_min);

    if (debut_contenu < 0 || (size_t)debut_contenu >= len_texte) {
        return NULL;
    }

    pos = (size_t)debut_contenu;
    size_t fin_contenu = len_texte;
    int ligne_vide_precedente = 1;

    while (pos < len_texte) {
        size_t debut_ligne = pos;
        size_t fin_ligne   = pos;
        while (fin_ligne < len_texte && texte[fin_ligne] != '\n') {
            fin_ligne++;
        }

        size_t len_ligne = fin_ligne - debut_ligne;
        char ligne[256]  = {0};

        if (len_ligne < 256) {
            memcpy(ligne, texte + debut_ligne, len_ligne);
            ligne[len_ligne] = '\0';
            trim(ligne);
        }

        size_t len_trim = strlen(ligne);

        if (len_trim == 0) {
            ligne_vide_precedente = 1;
        } else {
            if (ligne_vide_precedente &&
                len_trim <= 60 &&
                pos > (size_t)debut_contenu + 2) {
                fin_contenu = debut_ligne;
                break;
            }
            ligne_vide_precedente = 0;
        }
        pos = fin_ligne + 1;
    }

    if (fin_contenu <= (size_t)debut_contenu) return NULL;

    size_t taille = fin_contenu - (size_t)debut_contenu;
    char  *contenu = (char *)malloc(taille + 1);
    if (!contenu) return NULL;

    memcpy(contenu, texte + debut_contenu, taille);
    contenu[taille] = '\0';

    return contenu;
}

/* --------------------------------------------------------------------------
 * check_word_count_min
 * -------------------------------------------------------------------------- */
RuleResult check_word_count_min(const Rule *rule, const char *texte)
{
    // Correction : Test simplifié de l'ID
    const char *id = rule ? rule->id : "?";

    if (!rule || !texte) {
        return faire_resultat(id, STATUS_VIOLATION, "Paramètre invalide");
    }

    // Correction : Utilisation de param_wc et seuil
    const ParamWordCount *param = &rule->param_wc;

    char *contenu = extraire_section(texte, param->section);
    if (!contenu) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Section \"%s\" introuvable", param->section);
        return faire_resultat(id, STATUS_VIOLATION, msg);
    }

    size_t nb_mots = compter_mots(contenu);
    free(contenu);

    char msg[256];
    if (nb_mots < (size_t)param->seuil) {
        snprintf(msg, sizeof(msg),
                 "Section \"%s\" : %zu mot(s) (minimum requis : %d)",
                 param->section, nb_mots, param->seuil);
        return faire_resultat(id, STATUS_VIOLATION, msg);
    }

    snprintf(msg, sizeof(msg), "Section \"%s\" : %zu mot(s) ✓", param->section, nb_mots);
    return faire_resultat(id, STATUS_OK, msg);
}

/* --------------------------------------------------------------------------
 * check_word_count_max
 * -------------------------------------------------------------------------- */
RuleResult check_word_count_max(const Rule *rule, const char *texte)
{
    // Correction : Test simplifié de l'ID
    const char *id = rule ? rule->id : "?";

    if (!rule || !texte) {
        return faire_resultat(id, STATUS_VIOLATION, "Paramètre invalide");
    }

    // Correction : Utilisation de param_wc et seuil
    const ParamWordCount *param = &rule->param_wc;

    char *contenu = extraire_section(texte, param->section);
    if (!contenu) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Section \"%s\" introuvable", param->section);
        return faire_resultat(id, STATUS_WARNING, msg);
    }

    size_t nb_mots = compter_mots(contenu);
    free(contenu);

    char msg[256];
    if (nb_mots > (size_t)param->seuil) {
        snprintf(msg, sizeof(msg),
                 "Section \"%s\" : %zu mot(s) (maximum autorisé : %d)",
                 param->section, nb_mots, param->seuil);
        return faire_resultat(id, STATUS_VIOLATION, msg);
    }

    snprintf(msg, sizeof(msg), "Section \"%s\" : %zu mot(s) ✓", param->section, nb_mots);
    return faire_resultat(id, STATUS_OK, msg);
}