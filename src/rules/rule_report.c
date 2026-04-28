/*
 * rule_report.c — Génération du rapport de conformité
 *
 * Dispatch vers les vérificateurs selon le type de chaque règle.
 */

#include "rules/rule_report.h"
#include "rules/rule_engine.h"
#include "rules/checkers/section_checker.h"
#include "rules/checkers/regex_checker.h"
#include "rules/checkers/count_checker.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * Utilitaire : résultat par défaut pour les types non implémentés
 * -------------------------------------------------------------------------- */
static RuleResult resultat_non_implemente(const Rule *rule)
{
    RuleResult res;
    memset(&res, 0, sizeof(RuleResult)); // Initialise tout à zéro

    if (rule && rule->id) {
        strncpy(res.rule_id, rule->id, sizeof(res.rule_id) - 1);
    } else {
        strncpy(res.rule_id, "?", sizeof(res.rule_id) - 1);
    }
    
    res.statut = RULE_STATUS_PENDING;
    strncpy(res.message, "Vérification non disponible en Phase 2", sizeof(res.message) - 1);
    
    return res;
}

/* --------------------------------------------------------------------------
 * Dispatch : évalue une règle et retourne son résultat
 * -------------------------------------------------------------------------- */
static RuleResult evaluer_regle(const Rule *rule, const char *texte)
{
    if (!rule || !texte) {
        RuleResult res;
        memset(&res, 0, sizeof(RuleResult));
        strncpy(res.rule_id, "?", sizeof(res.rule_id) - 1);
        res.statut = STATUS_VIOLATION;
        strncpy(res.message, "Règle ou texte invalide", sizeof(res.message) - 1);
        return res;
    }

    switch (rule->type) {
        case CHECK_SECTION_EXISTS:
            return check_section_exists(rule, texte);

        case CHECK_SECTION_ORDER:
            return check_section_order(rule, texte);

        case CHECK_WORD_COUNT_MIN:
            return check_word_count_min(rule, texte);

        case CHECK_WORD_COUNT_MAX:
            return check_word_count_max(rule, texte);

        case CHECK_REGEX_FORBIDDEN:
            return check_regex_forbidden(rule, texte);

        case CHECK_REGEX_REQUIRED:
            return check_regex_required(rule, texte);

        case CHECK_HEADING_FORMAT:
            return check_heading_format(rule, texte);

        case CHECK_CITATION_PRESENT:
            return check_section_exists(rule, texte);

        case CHECK_LLM_SEMANTIC:
            return resultat_non_implemente(rule);

        default:
            return resultat_non_implemente(rule);
    }
}

/* --------------------------------------------------------------------------
 * rule_report_generer
 * -------------------------------------------------------------------------- */
RuleReport *rule_report_generer(const RuleSet *rs, const char *texte)
{
    if (!rs || !texte) return NULL;

    RuleReport *rapport = (RuleReport *)calloc(1, sizeof(RuleReport));
    if (!rapport) return NULL;

    if (rs->nb_regles == 0) {
        rapport->items = NULL;
        rapport->nb    = 0;
        return rapport;
    }

    rapport->items = (RuleResult *)malloc(rs->nb_regles * sizeof(RuleResult));
    if (!rapport->items) {
        free(rapport);
        return NULL;
    }

    rapport->nb      = rs->nb_regles;
    rapport->nb_ok   = 0;
    rapport->nb_warn = 0;
    rapport->nb_viol = 0;

    for (size_t i = 0; i < (size_t)rs->nb_regles; i++) {
        rapport->items[i] = evaluer_regle(&rs->regles[i], texte);

        switch (rapport->items[i].statut) {
            case STATUS_OK:
                rapport->nb_ok++; 
                break;
            case STATUS_WARNING:
                rapport->nb_warn++; 
                break;
            case STATUS_VIOLATION:
                rapport->nb_viol++; 
                break;
            default:
                break;
        }
    }

    return rapport;
}

/* --------------------------------------------------------------------------
 * rule_report_free
 * -------------------------------------------------------------------------- */
void rule_report_free(RuleReport *rapport)
{
    if (!rapport) return;

    /* * CORRECTION : On ne libère plus rule_id et message avec free() 
     * car ce sont des tableaux char[] fixes dans la structure RuleResult.
     */
    if (rapport->items) {
        free(rapport->items);
    }
    
    free(rapport);
}