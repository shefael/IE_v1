#ifndef LLM_LLM_INTERFACE_H
#define LLM_LLM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialise le contexte LLM avec un modèle donné */
int llm_init(const char* model_path);

/* Libère les ressources du LLM */
void llm_cleanup(void);

/* Envoie un prompt et récupère la réponse (bloquant, version simple) */
const char* llm_generate(const char* prompt);

#ifdef __cplusplus
}
#endif

#endif /* LLM_LLM_INTERFACE_H */