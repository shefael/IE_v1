#include "rules/rule_engine.h"
#include <stdlib.h>

struct RuleSet { int dummy; };
struct RuleResult { int dummy; };

RuleSet* ruleset_load(const char* json_path) {
    (void)json_path;
    RuleSet* rs = (RuleSet*)malloc(sizeof(RuleSet));
    if (rs) rs->dummy = 0;
    return rs;
}

RuleResult* ruleset_apply(const RuleSet* rs, const char* text) {
    (void)rs; (void)text;
    RuleResult* rr = (RuleResult*)malloc(sizeof(RuleResult));
    if (rr) rr->dummy = 0;
    return rr;
}

void ruleset_destroy(RuleSet* rs) {
    free(rs);
}

void ruleresult_destroy(RuleResult* rr) {
    free(rr);
}

size_t ruleresult_count(const RuleResult* rr) {
    (void)rr;
    return 0;
}