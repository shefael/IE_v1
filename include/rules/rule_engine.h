#ifndef RULES_RULE_ENGINE_H
#define RULES_RULE_ENGINE_H

#include <stddef.h>

/* -----------------------------------------------------------------------
 * Énumérations
 * ----------------------------------------------------------------------- */

/* Sévérité d'une règle */
typedef enum {
    RULE_SEVERITY_ERROR,    /* Erreur bloquante        */
    SEVERITY_WARNING,  /* Avertissement           */
    SEVERITY_INFO      /* Information             */
} RuleSeverity;

/* Statut d'évaluation d'une règle */
typedef enum {
    STATUS_OK,         /* Règle respectée         */
    STATUS_WARNING,    /* Partiellement respectée */
    STATUS_VIOLATION,  /* Règle violée            */
    RULE_STATUS_PENDING     /* Vérification en cours   */
} RuleStatus;

/* Type de vérificateur */
typedef enum {
    CHECK_SECTION_EXISTS,    /* Présence d'une section              */
    CHECK_SECTION_ORDER,     /* Ordre des sections                  */
    CHECK_WORD_COUNT_MIN,    /* Nombre de mots minimum              */
    CHECK_WORD_COUNT_MAX,    /* Nombre de mots maximum              */
    CHECK_REGEX_FORBIDDEN,   /* Expression régulière interdite      */
    CHECK_REGEX_REQUIRED,    /* Expression régulière obligatoire    */
    CHECK_HEADING_FORMAT,    /* Format des titres                   */
    CHECK_CITATION_PRESENT,  /* Présence d'une bibliographie        */
    CHECK_LLM_SEMANTIC,      /* Vérification sémantique via LLM     */
    CHECK_INCONNU            /* Type non reconnu (ignoré)           */
} CheckType;

/* -----------------------------------------------------------------------
 * Paramètres spécifiques par type de vérificateur
 * ----------------------------------------------------------------------- */

/* Paramètre pour word_count_min / word_count_max */
typedef struct {
    char    section[256]; /* Nom de la section ciblée */
    int     seuil;        /* Nombre de mots min ou max */
} ParamWordCount;

/* Paramètre pour heading_format */
typedef struct {
    int  niveau;          /* Niveau du titre (1..4)     */
    char format[64];      /* "uppercase", "lowercase"... */
} ParamHeadingFormat;

/* Paramètre pour section_order : tableau de noms de sections */
#define MAX_SECTIONS_ORDRE 16
typedef struct {
    char sections[MAX_SECTIONS_ORDRE][256];
    int  nb_sections;
} ParamSectionOrder;

/* -----------------------------------------------------------------------
 * Structure d'une règle
 * ----------------------------------------------------------------------- */
typedef struct {
    char          id[64];
    char          categorie[64];
    RuleSeverity  severite;
    char          description[512];
    CheckType     type;

    /*
     * Paramètre générique selon le type :
     * - CHECK_SECTION_EXISTS    → param_chaine  (nom de la section)
     * - CHECK_REGEX_FORBIDDEN   → param_chaine  (regex)
     * - CHECK_REGEX_REQUIRED    → param_chaine  (regex)
     * - CHECK_LLM_SEMANTIC      → param_chaine  (question au LLM)
     * - CHECK_CITATION_PRESENT  → param_chaine  (nom section biblio)
     * - CHECK_WORD_COUNT_MIN    → param_wc
     * - CHECK_WORD_COUNT_MAX    → param_wc
     * - CHECK_HEADING_FORMAT    → param_hf
     * - CHECK_SECTION_ORDER     → param_so
     */
    char              param_chaine[512];
    ParamWordCount    param_wc;
    ParamHeadingFormat param_hf;
    ParamSectionOrder  param_so;

    /* Flags optionnels */
    int  flag_insensible_casse; /* 1 si regex insensible à la casse */

    /* Section cible pour llm_semantic */
    char section_cible[256];
} Rule;

/* -----------------------------------------------------------------------
 * Métadonnées du fichier de règles
 * ----------------------------------------------------------------------- */
typedef struct {
    char document_type[256];
    char version[64];
    char auteur[256];
    char description[512];
} RuleMeta;

/* -----------------------------------------------------------------------
 * Ensemble de règles
 * ----------------------------------------------------------------------- */
#define MAX_REGLES 64

typedef struct {
    RuleMeta meta;
    Rule     regles[MAX_REGLES];
    size_t   nb_regles;
} RuleSet;

/* -----------------------------------------------------------------------
 * Résultat d'évaluation d'une règle
 * ----------------------------------------------------------------------- */
typedef struct {
    char       rule_id[64];
    RuleStatus statut;
    char       message[512];
    size_t     position_debut; /* Position dans le doc (0 si N/A) */
    size_t     position_fin;
} RuleResult;

/* -----------------------------------------------------------------------
 * API de libération mémoire
 * ----------------------------------------------------------------------- */

/*
 * Libère un RuleSet alloué par rules_parse_file().
 */
void ruleset_free(RuleSet* rs);

/*
 * Retourne le nom lisible d'un statut.
 */
const char* rulestatus_to_string(RuleStatus st);

/*
 * Retourne le nom lisible d'un type de vérificateur.
 */
const char* checktype_to_string(CheckType ct);

#endif /* RULES_RULE_ENGINE_H */