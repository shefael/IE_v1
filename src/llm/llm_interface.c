#include "llm/llm_interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/*
 * On inclut l'API C de llama.cpp.
 * Le header llama.h expose une API C pure.
 */
#include "llama.h"


/* -----------------------------------------------------------------------
 * État global du module LLM
 * ----------------------------------------------------------------------- */
static struct llama_model* g_modele   = NULL;
static struct llama_context* g_contexte = NULL;
static const struct llama_vocab* g_vocab = NULL; // Nouveau : gestion du vocabulaire
static char                  g_chemin_modele[MAX_PATH] = "";
static int                   g_n_threads = 4;
static int                   g_n_ctx     = 4096;

/* -----------------------------------------------------------------------
 * Initialisation
 * ----------------------------------------------------------------------- */
int llm_init(const char* chemin_modele, int n_threads, int n_ctx) {
    if (!chemin_modele || !*chemin_modele) {
        return LLM_ERR_MODELE_MANQUANT;
    }

    /* Vérifier que le fichier existe */
    DWORD attrs = GetFileAttributesA(chemin_modele);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[LLM] Modèle introuvable : %s\n", chemin_modele);
        return LLM_ERR_MODELE_MANQUANT;
    }

    /* Si un modèle est déjà chargé, le décharger d'abord */
    if (g_modele) {
        llm_shutdown();
    }

    /* Initialisation du backend llama.cpp */
    llama_backend_init();

    /* Paramètres du modèle */
    struct llama_model_params params_modele = llama_model_default_params();
    params_modele.n_gpu_layers = 0; // CPU uniquement

    fprintf(stderr, "[LLM] Chargement du modèle : %s\n", chemin_modele);
    
    // Remplacement : llama_load_model_from_file -> llama_model_load_from_file
    g_modele = llama_model_load_from_file(chemin_modele, params_modele);
    
    if (!g_modele) {
        fprintf(stderr, "[LLM] Échec du chargement du modèle.\n");
        llama_backend_free();
        return LLM_ERR_MODELE_INVALIDE;
    }

    /* Nouveau : Extraction du vocabulaire depuis le modèle */
    g_vocab = llama_model_get_vocab(g_modele);

    /* Paramètres du contexte */
    struct llama_context_params params_ctx = llama_context_default_params();
    params_ctx.n_ctx      = (uint32_t)(n_ctx > 0 ? n_ctx : 4096);
    params_ctx.n_threads  = (uint32_t)(n_threads > 0 ? n_threads : 4);
    params_ctx.n_threads_batch = params_ctx.n_threads;

    // Remplacement : llama_new_context_with_model -> llama_init_from_model
    g_contexte = llama_init_from_model(g_modele, params_ctx);
    
    if (!g_contexte) {
        fprintf(stderr, "[LLM] Échec de création du contexte.\n");
        llama_model_free(g_modele);
        g_modele = NULL;
        llama_backend_free();
        return LLM_ERR_MEMOIRE;
    }

    strncpy(g_chemin_modele, chemin_modele, MAX_PATH - 1);
    g_chemin_modele[MAX_PATH - 1] = '\0';
    g_n_threads = n_threads;
    g_n_ctx     = n_ctx;
    
    fprintf(stderr, "[LLM] Modèle chargé avec succès. Prêt.\n");
    return LLM_OK;
}

/* -----------------------------------------------------------------------
 * Arrêt
 * ----------------------------------------------------------------------- */
void llm_shutdown(void) {
    if (g_contexte) {
        llama_free(g_contexte);
        g_contexte = NULL;
    }
    if (g_modele) {
        // Remplacement : llama_free_model -> llama_model_free
        llama_model_free(g_modele);
        g_modele = NULL;
    }
    g_vocab = NULL;
    llama_backend_free();
    g_chemin_modele[0] = '\0';
    fprintf(stderr, "[LLM] Arrêt propre.\n");
}

/* -----------------------------------------------------------------------
 * État et Utilitaires
 * ----------------------------------------------------------------------- */
int llm_is_ready(void) {
    return (g_modele != NULL && g_contexte != NULL && g_vocab != NULL) ? 1 : 0;
}

const char* llm_get_chemin_modele(void) {
    if (!g_modele) return NULL;
    return g_chemin_modele;
}

void llm_reset_contexte(void) {
    if (!g_contexte) return;

    llama_memory_t mem = llama_get_memory(g_contexte);
    llama_memory_seq_rm(mem, (llama_seq_id)-1, -1, -1);

    fprintf(stderr, "[LLM] Cache KV réinitialisé.\n");
}

/* -----------------------------------------------------------------------
 * Génération de texte
 * ----------------------------------------------------------------------- */
int llm_generer(const char* prompt, char* buffer_sortie,
                size_t taille_buffer, int timeout_ms) {
    if (!llm_is_ready()) return LLM_ERR_NON_INITIALISE;
    if (!prompt || !buffer_sortie || taille_buffer == 0) return -1;

    int n_tokens_max = g_n_ctx;
    llama_token* tokens = (llama_token*)malloc(sizeof(llama_token) * (size_t)n_tokens_max);
    if (!tokens) return -1;

    // Remplacement : Utilise g_vocab au lieu de g_modele
    int n_tokens = llama_tokenize(
        g_vocab,
        prompt,
        (int32_t)strlen(prompt),
        tokens,
        n_tokens_max,
        1, // add_special
        1  // parse_special
    );

    if (n_tokens < 0) {
        fprintf(stderr, "[LLM] Erreur tokenisation.\n");
        free(tokens);
        return -1;
    }


    struct llama_batch batch = llama_batch_get_one(tokens, n_tokens);
    if (llama_decode(g_contexte, batch) != 0) {
        fprintf(stderr, "[LLM] Erreur lors du décodage du prompt.\n");
        free(tokens);
        return -1;
    }

    size_t pos_sortie    = 0;
    int    n_generes     = 0;
    int    n_max_generes = 512;
    DWORD  temps_debut   = GetTickCount();

    // Remplacement : llama_token_eos -> llama_vocab_eos
    llama_token token_eos = llama_vocab_eos(g_vocab);

    while (n_generes < n_max_generes) {
        if (timeout_ms > 0) {
            DWORD ecoule = GetTickCount() - temps_debut;
            if ((int)ecoule > timeout_ms) {
                fprintf(stderr, "[LLM] Timeout atteint.\n");
                break;
            }
        }

        struct llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(sampler, llama_sampler_init_greedy());

        llama_token token_suivant = llama_sampler_sample(sampler, g_contexte, -1);
        llama_sampler_free(sampler);

        // Remplacement : llama_token_is_eog -> llama_vocab_is_eog
        if (token_suivant == token_eos || llama_vocab_is_eog(g_vocab, token_suivant)) {
            break;
        }

        char morceau[256] = {0};
        // Utilisation du nom suggéré précédemment par votre compilateur
        int nb_octets = llama_token_to_piece(g_vocab, token_suivant, morceau, sizeof(morceau) - 1, 0, true);
/* ... */

        if (nb_octets > 0 && pos_sortie + (size_t)nb_octets < taille_buffer) {
            memcpy(buffer_sortie + pos_sortie, morceau, (size_t)nb_octets);
            pos_sortie += (size_t)nb_octets;
        }

        struct llama_batch batch_suivant = llama_batch_get_one(&token_suivant, 1);
        if (llama_decode(g_contexte, batch_suivant) != 0) {
            fprintf(stderr, "[LLM] Erreur de décodage token.\n");
            break;
        }

        n_generes++;
    }

    buffer_sortie[pos_sortie] = '\0';
    free(tokens);

    return (int)pos_sortie;
}