#ifndef RULES_RULE_ENGINE_H
#define RULES_RULE_ENGINE_H

#include <stddef.h>   /* pour size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Structures opaques */
typedef struct Rule Rule;
typedef struct RuleSet RuleSet;
typedef struct RuleResult RuleResult;

/* Charge un ensemble de règles depuis un fichier JSON */
RuleSet* ruleset_load(const char* json_path);

/* Applique un ensemble de règles au texte donné */
RuleResult* ruleset_apply(const RuleSet* rs, const char* text);

/* Libère un ensemble de règles */
void ruleset_destroy(RuleSet* rs);

/* Libère un résultat de règles */
void ruleresult_destroy(RuleResult* rr);

/* Récupère le nombre de problèmes trouvés */
size_t ruleresult_count(const RuleResult* rr);

#ifdef __cplusplus
}
#endif

#endif /* RULES_RULE_ENGINE_H */