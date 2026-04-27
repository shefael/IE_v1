#ifndef RULES_CHECKERS_COUNT_CHECKER_H
#define RULES_CHECKERS_COUNT_CHECKER_H

/*
 * count_checker.h — Vérificateurs de nombre de mots
 *
 * Implémente :
 *   - word_count_min : section doit contenir au moins N mots
 *   - word_count_max : section ne doit pas dépasser N mots
 */

#include "rules/rule_engine.h"

/*
 * Vérifie que la section ciblée contient au moins min_words mots.
 * STATUS_VIOLATION si le nombre de mots est insuffisant.
 */
RuleResult check_word_count_min(const Rule *rule, const char *texte);

/*
 * Vérifie que la section ciblée ne dépasse pas max_words mots.
 * STATUS_VIOLATION si le nombre de mots est excessif.
 */
RuleResult check_word_count_max(const Rule *rule, const char *texte);

#endif /* RULES_CHECKERS_COUNT_CHECKER_H */