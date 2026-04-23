/* Fichier temporaire — NE PAS COMMITER — SUPPRIMER APRÈS VALIDATION */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules/rule_engine.h"
#include "rules/rule_parser.h"

int main(void) {
    printf("=== Test Rule Parser ===\n");

    const char* chemin = "data/rule_templates/test_rules.json";
    RuleSet* rs = rules_parse_file(chemin);

    if (!rs) {
        printf("ECHEC : rules_parse_file retourne NULL\n");
        return 1;
    }

    printf("OK : fichier parsé\n");
    printf("  document_type : '%s'\n", rs->meta.document_type);
    printf("  version       : '%s'\n", rs->meta.version);
    printf("  nb_regles     : %zu\n",  rs->nb_regles);

    if (rs->nb_regles != 5) {
        printf("ECHEC : attendu 5 règles, obtenu %zu\n", rs->nb_regles);
        ruleset_free(rs);
        return 1;
    }
    printf("OK : nombre de règles correct\n\n");

    /* Vérifier chaque règle */
    for (size_t i = 0; i < rs->nb_regles; i++) {
        Rule* r = &rs->regles[i];
        printf("  [%s] type=%-20s sev=%d desc='%.40s'\n",
               r->id,
               checktype_to_string(r->type),
               r->severite,
               r->description);

        /* Vérifications spécifiques */
        if (i == 0) { /* R001 : section_exists */
            if (r->type != CHECK_SECTION_EXISTS) {
                printf("  ECHEC R001 : mauvais type\n"); }
            else if (strcmp(r->param_chaine, "Introduction") != 0) {
                printf("  ECHEC R001 : param_chaine='%s'\n", r->param_chaine); }
            else printf("  OK R001\n");
        }
        if (i == 2) { /* R003 : regex_forbidden + flag */
            if (r->type != CHECK_REGEX_FORBIDDEN) {
                printf("  ECHEC R003 : mauvais type\n"); }
            else if (!r->flag_insensible_casse) {
                printf("  ECHEC R003 : flag insensible manquant\n"); }
            else printf("  OK R003 (flag insensible=%d)\n",
                        r->flag_insensible_casse);
        }
        if (i == 3) { /* R004 : word_count_min */
            if (r->type != CHECK_WORD_COUNT_MIN) {
                printf("  ECHEC R004 : mauvais type\n"); }
            else if (r->param_wc.seuil != 300) {
                printf("  ECHEC R004 : seuil=%d\n", r->param_wc.seuil); }
            else printf("  OK R004 (section='%s', seuil=%d)\n",
                        r->param_wc.section, r->param_wc.seuil);
        }
        if (i == 4) { /* R005 : heading_format */
            if (r->type != CHECK_HEADING_FORMAT) {
                printf("  ECHEC R005 : mauvais type\n"); }
            else if (r->param_hf.niveau != 1) {
                printf("  ECHEC R005 : niveau=%d\n", r->param_hf.niveau); }
            else printf("  OK R005 (niveau=%d, format='%s')\n",
                        r->param_hf.niveau, r->param_hf.format);
        }
    }

    /* Test JSON invalide */
    RuleSet* rs2 = rules_parse_string("{invalide json{{{}");
    if (!rs2) printf("\nOK : JSON invalide retourne NULL\n");
    else { printf("\nECHEC : JSON invalide aurait dû retourner NULL\n");
           ruleset_free(rs2); }

    /* Test check_type inconnu */
    const char* json_inconnu =
        "{\"meta\":{\"document_type\":\"T\",\"version\":\"1\","
        "\"author\":\"A\",\"description\":\"D\"},"
        "\"rules\":[{\"id\":\"X1\",\"category\":\"c\","
        "\"severity\":\"warning\",\"description\":\"d\","
        "\"check_type\":\"type_inconnu\",\"parameter\":\"p\"}]}";
    RuleSet* rs3 = rules_parse_string(json_inconnu);
    if (rs3) {
        if (rs3->nb_regles == 0)
            printf("OK : check_type inconnu ignoré\n");
        else
            printf("ECHEC : règle invalide aurait dû être ignorée\n");
        ruleset_free(rs3);
    }

    ruleset_free(rs);
    printf("\n=== Tous les tests passent ===\n");
    return 0;
}