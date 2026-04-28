#ifndef RULES_RULE_REPORT_H
#define RULES_RULE_REPORT_H

/*
 * rule_report.h — Rapport de conformité global
 *
 * Orchestre tous les vérificateurs synchrones et produit
 * un RuleReport contenant un RuleResult par règle.
 *
 * Le debounce (2 secondes d'inactivité) est géré côté UI
 * via le TIMER_REGLES_ID. Ce module est purement synchrone.
 */

#include "rules/rule_engine.h"

/* --------------------------------------------------------------------------
 * Structure du rapport
 * -------------------------------------------------------------------------- */
typedef struct {
    RuleResult *items;   /* Tableau de résultats, un par règle */
    size_t      nb;      /* Nombre de résultats */
    int         nb_ok;   /* Nombre de règles conformes */
    int         nb_warn; /* Nombre d'avertissements */
    int         nb_viol; /* Nombre de violations */
} RuleReport;

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Génère le rapport de conformité complet.
 * Évalue toutes les règles du RuleSet sur le texte fourni.
 * Les règles llm_semantic sont ignorées (STATUS_PENDING) en Phase 2.
 *
 * Retourne un RuleReport alloué dynamiquement.
 * Libérer avec rule_report_free().
 */
RuleReport *rule_report_generer(const RuleSet *rs, const char *texte);

/*
 * Libère un RuleReport et tous ses résultats.
 */
void rule_report_free(RuleReport *rapport);

#endif /* RULES_RULE_REPORT_H */