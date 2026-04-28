#ifndef LLM_PROMPTS_H
#define LLM_PROMPTS_H

/*
 * prompts.h — Templates de prompts pour le LLM
 *
 * Tous les prompts demandent une réponse en JSON structuré.
 * Les fonctions retournent des buffers statiques ou alloués
 * dynamiquement selon la complexité du prompt.
 *
 * Format JSON attendu pour la correction grammaticale :
 * {
 *   "errors": [
 *     {
 *       "type": "subject_verb_agreement",
 *       "original": "les règles est",
 *       "corrected": "les règles sont",
 *       "explanation": "Accord sujet-verbe"
 *     }
 *   ]
 * }
 *
 * Format JSON attendu pour la reformulation :
 * {
 *   "reformulated": "texte reformulé",
 *   "explanation": "explication du changement"
 * }
 */

/*
 * Construit un prompt de correction grammaticale pour une phrase.
 * Retourne un buffer alloué dynamiquement (libérer avec free()).
 * Retourne NULL en cas d'erreur.
 */
char *prompt_correction_grammaire(const char *phrase);

/*
 * Construit un prompt de reformulation stylistique.
 * style : "formal", "simple", "academic"
 * Retourne un buffer alloué dynamiquement (libérer avec free()).
 */
char *prompt_reformuler(const char *texte, const char *style);

/*
 * Construit un prompt de vérification sémantique.
 * question    : question posée sur le texte (ex: "La problématique est-elle posée ?")
 * texte_section : contenu de la section à évaluer
 * Retourne un buffer alloué dynamiquement (libérer avec free()).
 */
char *prompt_verification_semantique(const char *question,
                                      const char *texte_section);

#endif /* LLM_PROMPTS_H */