/*
 * prompts.c — Implémentation des templates de prompts LLM
 *
 * Les prompts sont construits en format Mistral/Llama Instruct :
 *   [INST] instruction [/INST]
 *
 * On demande explicitement une réponse JSON uniquement,
 * sans texte libre avant ou après le JSON.
 */

#include "llm/prompts.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Taille maximale d'un prompt */
#define TAILLE_MAX_PROMPT 8192

/* --------------------------------------------------------------------------
 * prompt_correction_grammaire
 * -------------------------------------------------------------------------- */
char *prompt_correction_grammaire(const char *phrase)
{
    if (!phrase || phrase[0] == '\0') return NULL;

    char *buf = (char *)malloc(TAILLE_MAX_PROMPT);
    if (!buf) return NULL;

    snprintf(buf, TAILLE_MAX_PROMPT,
        "[INST] Tu es un correcteur grammatical expert en français académique.\n"
        "Analyse la phrase suivante et identifie toutes les erreurs grammaticales.\n"
        "Réponds UNIQUEMENT avec un objet JSON valide, sans texte avant ni après.\n"
        "Format de réponse obligatoire :\n"
        "{\n"
        "  \"errors\": [\n"
        "    {\n"
        "      \"type\": \"<type_erreur>\",\n"
        "      \"original\": \"<texte_fautif>\",\n"
        "      \"corrected\": \"<texte_corrigé>\",\n"
        "      \"explanation\": \"<explication_courte>\"\n"
        "    }\n"
        "  ]\n"
        "}\n"
        "Types d'erreurs possibles : subject_verb_agreement, "
        "gender_number_agreement, punctuation, repetition, anglicism.\n"
        "Si aucune erreur : retourner {\"errors\": []}\n\n"
        "Phrase à analyser : \"%s\" [/INST]",
        phrase
    );

    return buf;
}

/* --------------------------------------------------------------------------
 * prompt_reformuler
 * -------------------------------------------------------------------------- */
char *prompt_reformuler(const char *texte, const char *style)
{
    if (!texte || !style) return NULL;

    char *buf = (char *)malloc(TAILLE_MAX_PROMPT);
    if (!buf) return NULL;

    /* Description du style demandé */
    const char *desc_style;
    if (strcmp(style, "formal") == 0) {
        desc_style =
            "formel et professionnel, sans familiarités, "
            "avec un vocabulaire soutenu";
    } else if (strcmp(style, "simple") == 0) {
        desc_style =
            "simple et accessible, avec des phrases courtes "
            "et un vocabulaire courant";
    } else if (strcmp(style, "academic") == 0) {
        desc_style =
            "académique et scientifique, sans usage de la première "
            "personne du singulier, avec un vocabulaire précis et "
            "des formulations impersonnelles";
    } else {
        desc_style = "neutre et clair";
    }

    snprintf(buf, TAILLE_MAX_PROMPT,
        "[INST] Tu es un expert en rédaction académique française.\n"
        "Reformule le texte suivant dans un style %s.\n"
        "Réponds UNIQUEMENT avec un objet JSON valide, sans texte avant ni après.\n"
        "Format de réponse obligatoire :\n"
        "{\n"
        "  \"reformulated\": \"<texte_reformulé>\",\n"
        "  \"explanation\": \"<brève_explication_des_changements>\"\n"
        "}\n\n"
        "Texte à reformuler : \"%s\" [/INST]",
        desc_style,
        texte
    );

    return buf;
}

/* --------------------------------------------------------------------------
 * prompt_verification_semantique
 * -------------------------------------------------------------------------- */
char *prompt_verification_semantique(const char *question,
                                      const char *texte_section)
{
    if (!question || !texte_section) return NULL;

    char *buf = (char *)malloc(TAILLE_MAX_PROMPT);
    if (!buf) return NULL;

    snprintf(buf, TAILLE_MAX_PROMPT,
        "[INST] Tu es un expert en analyse de documents académiques français.\n"
        "Analyse le texte suivant et réponds à la question posée.\n"
        "Réponds UNIQUEMENT avec un objet JSON valide, sans texte avant ni après.\n"
        "Format de réponse obligatoire :\n"
        "{\n"
        "  \"reponse\": true,\n"
        "  \"confiance\": 0.85,\n"
        "  \"justification\": \"<explication_courte>\"\n"
        "}\n"
        "où \"reponse\" est true si la réponse à la question est oui, "
        "false sinon.\n"
        "\"confiance\" est un nombre entre 0.0 et 1.0.\n\n"
        "Question : %s\n\n"
        "Texte à analyser :\n%s [/INST]",
        question,
        texte_section
    );

    return buf;
}