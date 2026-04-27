#include "rules/rule_engine.h"
#include <stdlib.h>

void ruleset_free(RuleSet* rs) {
    /*
     * RuleSet utilise des tableaux statiques intégrés (pas de malloc
     * interne dans les structures Rule). La libération se limite donc
     * à libérer la structure elle-même si elle a été allouée sur le tas.
     * Le parser alloue le RuleSet avec malloc — on le libère ici.
     */
    free(rs);
}

const char* rulestatus_to_string(RuleStatus st) {
    switch (st) {
        case STATUS_OK:        return "Conforme";
        case STATUS_WARNING:   return "Avertissement";
        case STATUS_VIOLATION: return "Non conforme";
        case RULE_STATUS_PENDING:   return "En cours";
        default:               return "Inconnu";
    }
}

const char* checktype_to_string(CheckType ct) {
    switch (ct) {
        case CHECK_SECTION_EXISTS:   return "section_exists";
        case CHECK_SECTION_ORDER:    return "section_order";
        case CHECK_WORD_COUNT_MIN:   return "word_count_min";
        case CHECK_WORD_COUNT_MAX:   return "word_count_max";
        case CHECK_REGEX_FORBIDDEN:  return "regex_forbidden";
        case CHECK_REGEX_REQUIRED:   return "regex_required";
        case CHECK_HEADING_FORMAT:   return "heading_format";
        case CHECK_CITATION_PRESENT: return "citation_present";
        case CHECK_LLM_SEMANTIC:     return "llm_semantic";
        default:                     return "inconnu";
    }
}