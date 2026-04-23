#ifndef RULES_RULE_PARSER_H
#define RULES_RULE_PARSER_H

#include "rules/rule_engine.h"

/*
 * Parse un fichier JSON de règles et retourne un RuleSet alloué.
 * L'appelant doit libérer avec ruleset_free().
 * Retourne NULL en cas d'erreur (fichier manquant, JSON invalide).
 */
RuleSet* rules_parse_file(const char* chemin_fichier);

/*
 * Parse une chaîne JSON de règles (utile pour les tests).
 * Retourne NULL en cas d'erreur.
 */
RuleSet* rules_parse_string(const char* json_str);

#endif /* RULES_RULE_PARSER_H */