#ifndef RULES_CHECKERS_REGEX_CHECKER_H
#define RULES_CHECKERS_REGEX_CHECKER_H

/*
 * regex_checker.h — Vérificateurs regex via PCRE2
 *
 * Implémente :
 *   - regex_forbidden : l'expression ne doit PAS être trouvée
 *   - regex_required  : l'expression DOIT être trouvée
 */

#include "rules/rule_engine.h"

/*
 * Vérifie qu'une expression interdite n'est pas présente.
 * STATUS_VIOLATION si la regex trouve une correspondance.
 */
RuleResult check_regex_forbidden(const Rule *rule, const char *texte);

/*
 * Vérifie qu'une expression requise est présente.
 * STATUS_VIOLATION si la regex ne trouve aucune correspondance.
 */
RuleResult check_regex_required(const Rule *rule, const char *texte);

#endif /* RULES_CHECKERS_REGEX_CHECKER_H */