#include "rules/rule_parser.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Fonctions utilitaires internes
 * ----------------------------------------------------------------------- */

/* Copie sécurisée d'une chaîne JSON dans un buffer fixe */
static void copier_chaine_json(const cJSON* item, char* dst, size_t max) {
    if (!item || !cJSON_IsString(item) || !item->valuestring) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, item->valuestring, max - 1);
    dst[max - 1] = '\0';
}

/* Convertit la chaîne "severity" en RuleSeverity */
static RuleSeverity parser_severite(const char* s) {
    if (!s) return SEVERITY_WARNING;
    if (strcmp(s, "error")   == 0) return RULE_SEVERITY_ERROR;
    if (strcmp(s, "warning") == 0) return SEVERITY_WARNING;
    if (strcmp(s, "info")    == 0) return SEVERITY_INFO;
    return SEVERITY_WARNING;
}

/* Convertit la chaîne "check_type" en CheckType */
static CheckType parser_check_type(const char* s) {
    if (!s) return CHECK_INCONNU;
    if (strcmp(s, "section_exists")   == 0) return CHECK_SECTION_EXISTS;
    if (strcmp(s, "section_order")    == 0) return CHECK_SECTION_ORDER;
    if (strcmp(s, "word_count_min")   == 0) return CHECK_WORD_COUNT_MIN;
    if (strcmp(s, "word_count_max")   == 0) return CHECK_WORD_COUNT_MAX;
    if (strcmp(s, "regex_forbidden")  == 0) return CHECK_REGEX_FORBIDDEN;
    if (strcmp(s, "regex_required")   == 0) return CHECK_REGEX_REQUIRED;
    if (strcmp(s, "heading_format")   == 0) return CHECK_HEADING_FORMAT;
    if (strcmp(s, "citation_present") == 0) return CHECK_CITATION_PRESENT;
    if (strcmp(s, "llm_semantic")     == 0) return CHECK_LLM_SEMANTIC;
    return CHECK_INCONNU;
}

/*
 * Parse le champ "parameter" d'une règle selon son type.
 * La structure du paramètre dépend du check_type.
 */
static void parser_parametre(const cJSON* param, Rule* regle) {
    if (!param) return;

    switch (regle->type) {

        case CHECK_SECTION_EXISTS:
        case CHECK_REGEX_FORBIDDEN:
        case CHECK_REGEX_REQUIRED:
        case CHECK_CITATION_PRESENT:
        case CHECK_LLM_SEMANTIC:
            /* Paramètre = chaîne simple */
            if (cJSON_IsString(param)) {
                strncpy(regle->param_chaine, param->valuestring,
                        sizeof(regle->param_chaine) - 1);
                regle->param_chaine[sizeof(regle->param_chaine) - 1] = '\0';
            }
            break;

        case CHECK_SECTION_ORDER:
            /* Paramètre = tableau de chaînes */
            if (cJSON_IsArray(param)) {
                int idx = 0;
                cJSON* item;
                cJSON_ArrayForEach(item, param) {
                    if (idx >= MAX_SECTIONS_ORDRE) break;
                    if (cJSON_IsString(item)) {
                        strncpy(regle->param_so.sections[idx],
                                item->valuestring,
                                255);
                        regle->param_so.sections[idx][255] = '\0';
                        idx++;
                    }
                }
                regle->param_so.nb_sections = idx;
            }
            break;

        case CHECK_WORD_COUNT_MIN:
        case CHECK_WORD_COUNT_MAX:
            /* Paramètre = objet { "section": "...", "min_words"/"max_words": N } */
            if (cJSON_IsObject(param)) {
                cJSON* sec = cJSON_GetObjectItemCaseSensitive(param, "section");
                if (sec && cJSON_IsString(sec)) {
                    strncpy(regle->param_wc.section, sec->valuestring,
                            sizeof(regle->param_wc.section) - 1);
                    regle->param_wc.section[sizeof(regle->param_wc.section)-1] = '\0';
                }

                /* Chercher min_words ou max_words */
                cJSON* nb = cJSON_GetObjectItemCaseSensitive(param, "min_words");
                if (!nb) nb = cJSON_GetObjectItemCaseSensitive(param, "max_words");
                if (nb && cJSON_IsNumber(nb)) {
                    regle->param_wc.seuil = (int)nb->valuedouble;
                }
            }
            break;

        case CHECK_HEADING_FORMAT:
            /* Paramètre = objet { "level": N, "case": "uppercase" } */
            if (cJSON_IsObject(param)) {
                cJSON* lvl = cJSON_GetObjectItemCaseSensitive(param, "level");
                if (lvl && cJSON_IsNumber(lvl)) {
                    regle->param_hf.niveau = (int)lvl->valuedouble;
                }
                cJSON* cas = cJSON_GetObjectItemCaseSensitive(param, "case");
                if (cas && cJSON_IsString(cas)) {
                    strncpy(regle->param_hf.format, cas->valuestring,
                            sizeof(regle->param_hf.format) - 1);
                    regle->param_hf.format[sizeof(regle->param_hf.format)-1] = '\0';
                }
            }
            break;

        default:
            break;
    }
}

/* -----------------------------------------------------------------------
 * Parsing d'un objet JSON racine → RuleSet
 * ----------------------------------------------------------------------- */
static RuleSet* parser_json(cJSON* racine) {
    if (!racine) return NULL;

    RuleSet* rs = (RuleSet*)calloc(1, sizeof(RuleSet));
    if (!rs) return NULL;

    /* --- Métadonnées --- */
    cJSON* meta = cJSON_GetObjectItemCaseSensitive(racine, "meta");
    if (meta && cJSON_IsObject(meta)) {
        copier_chaine_json(
            cJSON_GetObjectItemCaseSensitive(meta, "document_type"),
            rs->meta.document_type, sizeof(rs->meta.document_type));
        copier_chaine_json(
            cJSON_GetObjectItemCaseSensitive(meta, "version"),
            rs->meta.version, sizeof(rs->meta.version));
        copier_chaine_json(
            cJSON_GetObjectItemCaseSensitive(meta, "author"),
            rs->meta.auteur, sizeof(rs->meta.auteur));
        copier_chaine_json(
            cJSON_GetObjectItemCaseSensitive(meta, "description"),
            rs->meta.description, sizeof(rs->meta.description));
    }

    /* --- Tableau de règles --- */
    cJSON* regles_json = cJSON_GetObjectItemCaseSensitive(racine, "rules");
    if (!regles_json || !cJSON_IsArray(regles_json)) {
        fprintf(stderr, "[RULES] Champ 'rules' manquant ou invalide.\n");
        free(rs);
        return NULL;
    }

    cJSON* regle_json;
    cJSON_ArrayForEach(regle_json, regles_json) {
        if (rs->nb_regles >= MAX_REGLES) {
            fprintf(stderr, "[RULES] Limite de %d règles atteinte.\n",
                    MAX_REGLES);
            break;
        }

        if (!cJSON_IsObject(regle_json)) continue;

        Rule* r = &rs->regles[rs->nb_regles];

        /* Champ obligatoire : id */
        cJSON* id_json = cJSON_GetObjectItemCaseSensitive(regle_json, "id");
        if (!id_json || !cJSON_IsString(id_json)) {
            fprintf(stderr,
                    "[RULES] Règle sans 'id' ignorée (index %zu).\n",
                    rs->nb_regles);
            continue;
        }
        copier_chaine_json(id_json, r->id, sizeof(r->id));

        /* Champs optionnels */
        copier_chaine_json(
            cJSON_GetObjectItemCaseSensitive(regle_json, "category"),
            r->categorie, sizeof(r->categorie));

        copier_chaine_json(
            cJSON_GetObjectItemCaseSensitive(regle_json, "description"),
            r->description, sizeof(r->description));

        /* Sévérité */
        cJSON* sev = cJSON_GetObjectItemCaseSensitive(regle_json, "severity");
        r->severite = parser_severite(
            (sev && cJSON_IsString(sev)) ? sev->valuestring : NULL);

        /* Type de vérificateur */
        cJSON* ct = cJSON_GetObjectItemCaseSensitive(regle_json, "check_type");
        const char* ct_str = (ct && cJSON_IsString(ct)) ? ct->valuestring : NULL;
        r->type = parser_check_type(ct_str);

        if (r->type == CHECK_INCONNU) {
            fprintf(stderr,
                    "[RULES] Type inconnu '%s' pour règle '%s' — ignorée.\n",
                    ct_str ? ct_str : "(null)", r->id);
            continue;
        }

        /* Flag insensible à la casse */
        cJSON* flags = cJSON_GetObjectItemCaseSensitive(regle_json, "flags");
        if (flags && cJSON_IsString(flags) &&
            strcmp(flags->valuestring, "case_insensitive") == 0) {
            r->flag_insensible_casse = 1;
        }

        /* Section cible (pour llm_semantic) */
        cJSON* ts = cJSON_GetObjectItemCaseSensitive(regle_json, "target_section");
        if (ts && cJSON_IsString(ts)) {
            strncpy(r->section_cible, ts->valuestring,
                    sizeof(r->section_cible) - 1);
            r->section_cible[sizeof(r->section_cible) - 1] = '\0';
        }

        /* Paramètre */
        cJSON* param = cJSON_GetObjectItemCaseSensitive(regle_json, "parameter");
        parser_parametre(param, r);

        rs->nb_regles++;
    }

    fprintf(stderr, "[RULES] %zu règle(s) chargée(s) depuis '%s'.\n",
            rs->nb_regles, rs->meta.document_type);

    return rs;
}

/* -----------------------------------------------------------------------
 * API publique
 * ----------------------------------------------------------------------- */

RuleSet* rules_parse_file(const char* chemin_fichier) {
    if (!chemin_fichier) return NULL;

    FILE* f = fopen(chemin_fichier, "r");
    if (!f) {
        fprintf(stderr, "[RULES] Impossible d'ouvrir : %s\n", chemin_fichier);
        return NULL;
    }

    /* Lire tout le contenu */
    fseek(f, 0, SEEK_END);
    long taille = ftell(f);
    rewind(f);

    if (taille <= 0) {
        fclose(f);
        return NULL;
    }

    char* contenu = (char*)malloc((size_t)taille + 1);
    if (!contenu) {
        fclose(f);
        return NULL;
    }

    size_t lu = fread(contenu, 1, (size_t)taille, f);
    contenu[lu] = '\0';
    fclose(f);

    RuleSet* rs = rules_parse_string(contenu);
    free(contenu);
    return rs;
}

RuleSet* rules_parse_string(const char* json_str) {
    if (!json_str) return NULL;

    cJSON* racine = cJSON_Parse(json_str);
    if (!racine) {
        const char* err = cJSON_GetErrorPtr();
        fprintf(stderr, "[RULES] Erreur JSON : %s\n", err ? err : "inconnue");
        return NULL;
    }

    RuleSet* rs = parser_json(racine);
    cJSON_Delete(racine);
    return rs;
}