/*
 * regex_checker.c — Vérificateurs regex via PCRE2
 *
 * La regex est compilée à chaque appel (les règles sont évaluées
 * périodiquement, pas en temps réel token par token).
 * Pour les performances, une optimisation future pourrait stocker
 * la regex compilée dans la structure Rule.
 */

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "rules/checkers/regex_checker.h"
#include "rules/rule_engine.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * Utilitaire : construire un RuleResult
 * -------------------------------------------------------------------------- */
static RuleResult faire_resultat(const char *rule_id,
                                  RuleStatus statut,
                                  const char *message)
{
    RuleResult res;
    if (rule_id) {
    strncpy(res.rule_id, rule_id, sizeof(res.rule_id) - 1);
    res.rule_id[sizeof(res.rule_id) - 1] = '\0'; // Assure la terminaison nulle
} else {
    strncpy(res.rule_id, "?", sizeof(res.rule_id) - 1);
}
    res.statut = statut;
    if (message) {
    strncpy(res.message, message, sizeof(res.message) - 1);
    res.message[sizeof(res.message) - 1] = '\0';
} else {
    res.message[0] = '\0';
}
    return res;
}

/* --------------------------------------------------------------------------
 * Utilitaire : compiler et exécuter une regex PCRE2
 *
 * Retourne 1 si la regex trouve au moins une correspondance.
 * Retourne 0 si aucune correspondance.
 * Retourne -1 en cas d'erreur de compilation.
 *
 * out_debut et out_longueur sont remplis avec la première correspondance
 * si une correspondance est trouvée.
 * -------------------------------------------------------------------------- */
static int executer_regex(const char *pattern,
                           int case_insensitive,
                           const char *texte,
                           size_t *out_debut,
                           size_t *out_longueur)
{
    if (!pattern || !texte) return -1;

    int        errcode;
    PCRE2_SIZE erroffset;
    uint32_t   options = PCRE2_UTF;

    if (case_insensitive) {
        options |= PCRE2_CASELESS;
    }

    pcre2_code *re = pcre2_compile(
        (PCRE2_SPTR)pattern,
        PCRE2_ZERO_TERMINATED,
        options,
        &errcode,
        &erroffset,
        NULL
    );

    if (!re) {
        /* Regex invalide : afficher l'erreur et retourner -1 */
        PCRE2_UCHAR buf[256];
        pcre2_get_error_message(errcode, buf, sizeof(buf));
        fprintf(stderr, "[regex_checker] Erreur compilation à %zu : %s\n",
                erroffset, (char *)buf);
        return -1;
    }

    pcre2_match_data *match_data =
        pcre2_match_data_create_from_pattern(re, NULL);

    if (!match_data) {
        pcre2_code_free(re);
        return -1;
    }

    size_t len_texte = strlen(texte);
    int rc = pcre2_match(
        re,
        (PCRE2_SPTR)texte,
        len_texte,
        0,
        0,
        match_data,
        NULL
    );

    int trouve = 0;
    if (rc > 0) {
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        if (out_debut)    *out_debut    = (size_t)ovector[0];
        if (out_longueur) *out_longueur = (size_t)(ovector[1] - ovector[0]);
        trouve = 1;
    } else if (rc == PCRE2_ERROR_NOMATCH) {
        trouve = 0;
    } else {
        fprintf(stderr, "[regex_checker] Erreur matching : %d\n", rc);
        trouve = 0;
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return trouve;
}

/*
 * Extrait un extrait de contexte autour de la correspondance.
 * Retourne une chaîne statique (usage immédiat uniquement).
 */
static const char *extraire_contexte(const char *texte,
                                      size_t debut,
                                      size_t longueur)
{
    static char buf[128];
    size_t len_texte = strlen(texte);

    /* Début du contexte : 20 caractères avant */
    size_t ctx_debut = (debut >= 20) ? debut - 20 : 0;
    /* Fin du contexte : 40 caractères après la fin de la correspondance */
    size_t ctx_fin   = debut + longueur + 40;
    if (ctx_fin > len_texte) ctx_fin = len_texte;

    size_t ctx_len = ctx_fin - ctx_debut;
    if (ctx_len >= sizeof(buf)) ctx_len = sizeof(buf) - 1;

    memcpy(buf, texte + ctx_debut, ctx_len);
    buf[ctx_len] = '\0';

    /* Remplacer les sauts de ligne par des espaces pour l'affichage */
    for (size_t i = 0; i < ctx_len; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') buf[i] = ' ';
    }

    return buf;
}

/* --------------------------------------------------------------------------
 * check_regex_forbidden
 * -------------------------------------------------------------------------- */
RuleResult check_regex_forbidden(const Rule *rule, const char *texte)
{
    const char *id = rule ? rule->id : "?";

    if (!rule || !texte) {
        return faire_resultat(id, STATUS_VIOLATION,
                              "Paramètre invalide");
    }

    const char *pattern = rule->param_chaine;
    if (!pattern || pattern[0] == '\0') {
        return faire_resultat(id, STATUS_WARNING,
                              "Expression régulière manquante");
    }

    size_t debut    = 0;
    size_t longueur = 0;
    int    rc       = executer_regex(pattern,
                                     rule->flag_insensible_casse,
                                     texte,
                                     &debut,
                                     &longueur);

    if (rc < 0) {
        return faire_resultat(id, STATUS_WARNING,
                              "Expression régulière invalide");
    }

    if (rc == 1) {
        /* Correspondance trouvée → violation */
        char msg[256];
        const char *contexte = extraire_contexte(texte, debut, longueur);
        snprintf(msg, sizeof(msg),
                 "Expression interdite trouvée : \"...%s...\"",
                 contexte);
        return faire_resultat(id, STATUS_VIOLATION, msg);
    }

    return faire_resultat(id, STATUS_OK,
                          "Expression interdite absente");
}

/* --------------------------------------------------------------------------
 * check_regex_required
 * -------------------------------------------------------------------------- */
RuleResult check_regex_required(const Rule *rule, const char *texte)
{
    const char *id = rule ? rule->id : "?";

    if (!rule || !texte) {
        return faire_resultat(id, STATUS_VIOLATION,
                              "Paramètre invalide");
    }

    const char *pattern = rule->param_chaine;
    if (!pattern || pattern[0] == '\0') {
        return faire_resultat(id, STATUS_WARNING,
                              "Expression régulière manquante");
    }

    size_t debut    = 0;
    size_t longueur = 0;
    int    rc       = executer_regex(pattern,
                                     rule->flag_insensible_casse,
                                     texte,
                                     &debut,
                                     &longueur);

    if (rc < 0) {
        return faire_resultat(id, STATUS_WARNING,
                              "Expression régulière invalide");
    }

    if (rc == 0) {
        /* Aucune correspondance → violation */
        return faire_resultat(id, STATUS_VIOLATION,
                              "Expression requise absente du document");
    }

    char msg[256];
    const char *contexte = extraire_contexte(texte, debut, longueur);
    snprintf(msg, sizeof(msg),
             "Expression requise trouvée : \"...%s...\"",
             contexte);
    return faire_resultat(id, STATUS_OK, msg);
}