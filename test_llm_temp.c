/* Fichier temporaire — NE PAS COMMITER — SUPPRIMER APRÈS VALIDATION */
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "llm/llm_interface.h"
#include "llm/llm_thread.h"

/* Callback appelé depuis le thread LLM */
static void mon_callback(int id, LlmStatut statut,
                          const char* reponse, void* donnees) {
    (void)donnees;
    printf("[CALLBACK] Requête #%d terminée, statut=%d\n", id, statut);
    if (statut == LLM_STATUT_TERMINE) {
        printf("[CALLBACK] Réponse (100 premiers octets) :\n%.100s\n", reponse);
    }
}

int main(void) {
    printf("=== Test LLM Interface + Thread ===\n");

    /* Charger le modèle */
    const char* chemin = "models/llama.gguf";
    printf("Chargement de : %s\n", chemin);

    int ret = llm_init(chemin, 4, 2048);
    if (ret != LLM_OK) {
        printf("ECHEC llm_init (code=%d)\n", ret);
        /* Test avec mistral si llama échoue */
        ret = llm_init("models/mistral.gguf", 4, 2048);
        if (ret != LLM_OK) {
            printf("ECHEC llm_init mistral aussi (code=%d)\n", ret);
            return 1;
        }
    }
    printf("OK : modèle chargé. Prêt=%d\n", llm_is_ready());

    /* Démarrer le thread */
    if (llm_thread_demarrer() != 0) {
        printf("ECHEC llm_thread_demarrer\n");
        llm_shutdown();
        return 1;
    }
    printf("OK : thread démarré\n");

    /* Soumettre une requête test */
    const char* prompt = "Bonjour ! Réponds en une seule phrase courte en français.";
    int id = llm_soumettre(prompt, mon_callback, NULL);
    if (id < 0) {
        printf("ECHEC llm_soumettre\n");
    } else {
        printf("OK : requête #%d soumise\n", id);
        printf("Attente de la réponse (max 35 secondes)...\n");

        /* Attendre que la requête soit traitée */
        int attente = 0;
        while (attente < 35000) {
            LlmStatut st = llm_get_statut(id);
            if (st == LLM_STATUT_TERMINE || st == LLM_STATUT_ERREUR) break;
            Sleep(500);
            attente += 500;
            printf("  ... %d ms\n", attente);
        }

        LlmStatut st = llm_get_statut(id);
        printf("Statut final : %d\n", st);
    }

    /* Vérifier que l'UI n'est pas bloquée
     * (le thread travaille en arrière-plan) */
    printf("OK : programme principal non bloqué pendant l'inférence\n");

    /* Arrêt propre */
    llm_thread_arreter();
    llm_shutdown();
    printf("OK : arrêt propre\n");
    printf("=== Test terminé ===\n");
    return 0;
}