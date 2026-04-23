#ifndef LLM_INTERFACE_H
#define LLM_INTERFACE_H

#include <stddef.h>

/* Codes de retour */
#define LLM_OK                   0
#define LLM_ERR_MODELE_MANQUANT -1
#define LLM_ERR_MEMOIRE         -2
#define LLM_ERR_MODELE_INVALIDE -3
#define LLM_ERR_NON_INITIALISE  -4

/*
 * Initialise le moteur LLM : charge le modèle GGUF en mémoire.
 * - chemin_modele : chemin UTF-8 vers le fichier .gguf
 * - n_threads     : nombre de threads CPU pour l'inférence
 * - n_ctx         : taille du contexte en tokens
 *
 * Retourne LLM_OK en cas de succès, un code d'erreur sinon.
 * Cette fonction peut prendre 5 à 15 secondes.
 */
int llm_init(const char* chemin_modele, int n_threads, int n_ctx);

/*
 * Libère toutes les ressources du LLM (modèle + contexte).
 * Doit être appelé avant la fermeture de l'application.
 */
void llm_shutdown(void);

/*
 * Retourne 1 si le LLM est chargé et prêt, 0 sinon.
 */
int llm_is_ready(void);

/*
 * Génère une réponse synchrone à un prompt.
 * - prompt       : texte d'entrée (UTF-8)
 * - buffer_sortie: buffer alloué par l'appelant pour la réponse
 * - taille_buffer: taille max du buffer en octets
 * - timeout_ms   : timeout en millisecondes (0 = pas de timeout)
 *
 * Retourne le nombre d'octets écrits, ou -1 en cas d'erreur.
 * ATTENTION : cette fonction est bloquante — utiliser via llm_thread.
 */
int llm_generer(const char* prompt, char* buffer_sortie,
                size_t taille_buffer, int timeout_ms);

/*
 * Réinitialise le cache KV du contexte sans recharger le modèle.
 * Utile entre deux sessions d'analyse indépendantes.
 */
void llm_reset_contexte(void);

/*
 * Retourne le chemin du modèle actuellement chargé.
 * Retourne NULL si aucun modèle n'est chargé.
 */
const char* llm_get_chemin_modele(void);

#endif /* LLM_INTERFACE_H */