#ifndef LLM_THREAD_H
#define LLM_THREAD_H

#include <stddef.h>
#include <windows.h>

/* Taille maximale d'un prompt ou d'une réponse */
#define LLM_TAILLE_MAX_PROMPT   8192
#define LLM_TAILLE_MAX_REPONSE  4096

/* Taille de la file de requêtes */
#define LLM_TAILLE_FILE         16

/* Timeout par défaut en millisecondes */
#define LLM_TIMEOUT_MS          30000

/* Statuts d'une requête */
typedef enum {
    LLM_STATUT_EN_ATTENTE,   /* Dans la file, pas encore traitée  */
    LLM_STATUT_EN_COURS,     /* En cours de traitement            */
    LLM_STATUT_TERMINE,      /* Réponse disponible                */
    LLM_STATUT_ERREUR,       /* Erreur ou timeout                 */
    LLM_STATUT_ANNULE        /* Annulée avant traitement          */
} LlmStatut;

/* Callback appelé quand une requête est terminée */
typedef void (*LlmCallback)(int id_requete, LlmStatut statut,
                             const char* reponse, void* donnees_utilisateur);

/* Structure d'une requête LLM */
typedef struct {
    int        id;                              /* Identifiant unique        */
    char       prompt[LLM_TAILLE_MAX_PROMPT];   /* Texte d'entrée            */
    char       reponse[LLM_TAILLE_MAX_REPONSE]; /* Réponse générée           */
    LlmStatut  statut;                          /* État courant              */
    LlmCallback callback;                       /* Fonction de rappel        */
    void*      donnees_utilisateur;             /* Données passées au callback*/
} LlmRequete;

/*
 * Démarre le thread worker LLM.
 * Le modèle doit être chargé via llm_init() AVANT d'appeler cette fonction.
 * Retourne 0 en cas de succès, -1 sinon.
 */
int llm_thread_demarrer(void);

/*
 * Arrête proprement le thread worker.
 * Attend que le thread se termine (max 5 secondes).
 */
void llm_thread_arreter(void);

/*
 * Soumet une requête dans la file.
 * - prompt    : texte du prompt (UTF-8)
 * - callback  : fonction appelée quand la réponse est prête (peut être NULL)
 * - donnees   : pointeur passé tel quel au callback
 *
 * Retourne l'ID unique de la requête (>= 1), ou -1 si la file est pleine.
 */
int llm_soumettre(const char* prompt, LlmCallback callback, void* donnees);

/*
 * Consulte le statut d'une requête par son ID.
 * Retourne LLM_STATUT_ERREUR si l'ID est inconnu.
 */
LlmStatut llm_get_statut(int id_requete);

/*
 * Copie la réponse d'une requête terminée dans 'buffer'.
 * Retourne 0 en cas de succès, -1 si la requête n'est pas terminée.
 */
int llm_get_reponse(int id_requete, char* buffer, size_t taille_buffer);

/*
 * Vide la file des requêtes en attente (pas celle en cours).
 */
void llm_vider_file(void);

#endif /* LLM_THREAD_H */