#ifndef RULES_CHECKERS_SECTION_CHECKER_H
#define RULES_CHECKERS_SECTION_CHECKER_H

/*
 * section_checker.h — Vérificateurs de structure de document
 *
 * Implémente :
 *   - section_exists   : présence d'un titre de section
 *   - section_order    : ordre des sections
 *   - heading_format   : format des titres (majuscules, etc.)
 */

#include "rules/rule_engine.h"

/*
 * Vérifie qu'une section dont le titre est dans rule->parameter
 * est présente dans le texte.
 */
RuleResult check_section_exists(const Rule *rule, const char *texte);

/*
 * Vérifie que les sections apparaissent dans l'ordre défini
 * par le tableau de chaînes dans rule->parameter.
 */
RuleResult check_section_order(const Rule *rule, const char *texte);

/*
 * Vérifie que les titres du niveau demandé respectent
 * le format attendu (ex: uppercase pour H1).
 */
RuleResult check_heading_format(const Rule *rule, const char *texte);

#endif /* RULES_CHECKERS_SECTION_CHECKER_H */