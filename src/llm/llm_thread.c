#include "llm/llm_thread.h"
#include "llm/llm_interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* -----------------------------------------------------------------------
 * File FIFO circulaire de requêtes
 * ----------------------------------------------------------------------- */
static LlmRequete  g_file[LLM_TAILLE_FILE];
static int         g_tete     = 0;  /* Index de lecture  */
static int         g_queue    = 0;  /* Index d'écriture  */
static int         g_nb       = 0;  /* Nombre d'éléments */
static int         g_id_prochain = 1; /* Compteur d'IDs  */

/* -----------------------------------------------------------------------
 * Synchronisation
 * ----------------------------------------------------------------------- */
static CRITICAL_SECTION g_cs;        /* Protège la file           */
static HANDLE           g_evt_tache; /* Signalé quand tâche dispo */
static HANDLE           g_thread;    /* Handle du thread worker   */
static volatile int     g_arreter = 0; /* Flag d'arrêt            */

/* -----------------------------------------------------------------------
 * Fonction du thread worker
 * ----------------------------------------------------------------------- */
static DWORD WINAPI thread_llm(LPVOID param) {
    (void)param;
    fprintf(stderr, "[LLM-THREAD] Démarré.\n");

    while (!g_arreter) {
        /* Attendre qu'une tâche soit disponible (max 500ms) */
        DWORD ret = WaitForSingleObject(g_evt_tache, 500);

        if (g_arreter) break;
        if (ret != WAIT_OBJECT_0) continue; /* Timeout → reboucler */

        /* Extraire la requête en tête de file */
        LlmRequete requete_locale;
        int index_local = -1;

        EnterCriticalSection(&g_cs);
        if (g_nb > 0) {
            index_local = g_tete;
            /* Copie locale pour travailler hors du verrou */
            requete_locale = g_file[g_tete];
            g_file[g_tete].statut = LLM_STATUT_EN_COURS;
            /* Ne pas avancer la tête ici — on la marque EN_COURS */
        }
        LeaveCriticalSection(&g_cs);

        if (index_local < 0) continue;

        fprintf(stderr, "[LLM-THREAD] Traitement requête #%d\n",
                requete_locale.id);

        /* Générer la réponse (bloquant, avec timeout) */
        char buffer_rep[LLM_TAILLE_MAX_REPONSE];
        int nb = llm_generer(
            requete_locale.prompt,
            buffer_rep,
            LLM_TAILLE_MAX_REPONSE,
            LLM_TIMEOUT_MS
        );

        /* Mettre à jour le statut et la réponse dans la file */
        EnterCriticalSection(&g_cs);

        if (nb >= 0) {
            strncpy(g_file[index_local].reponse, buffer_rep,
                    LLM_TAILLE_MAX_REPONSE - 1);
            g_file[index_local].reponse[LLM_TAILLE_MAX_REPONSE - 1] = '\0';
            g_file[index_local].statut = LLM_STATUT_TERMINE;
        } else {
            g_file[index_local].reponse[0] = '\0';
            g_file[index_local].statut = LLM_STATUT_ERREUR;
        }

        /* Sauvegarder le callback avant de libérer le verrou */
        LlmCallback cb  = g_file[index_local].callback;
        void*       dat = g_file[index_local].donnees_utilisateur;
        int         id  = g_file[index_local].id;
        LlmStatut   st  = g_file[index_local].statut;
        const char* rep = g_file[index_local].reponse;

        /* Avancer la tête de file */
        g_tete = (g_tete + 1) % LLM_TAILLE_FILE;
        g_nb--;

        LeaveCriticalSection(&g_cs);

        fprintf(stderr, "[LLM-THREAD] Requête #%d terminée (statut=%d)\n",
                id, st);

        /*
         * Appel du callback HORS du verrou.
         * Le callback ne doit PAS appeler llm_soumettre() directement
         * depuis ce thread — utiliser PostMessage() vers l'UI si besoin.
         */
        if (cb) {
            cb(id, st, rep, dat);
        }
    }

    fprintf(stderr, "[LLM-THREAD] Arrêt du thread.\n");
    return 0;
}

/* -----------------------------------------------------------------------
 * API publique
 * ----------------------------------------------------------------------- */

int llm_thread_demarrer(void) {
    if (g_thread) return 0; /* Déjà démarré */

    InitializeCriticalSection(&g_cs);

    /* Événement auto-reset : signalé quand une tâche est ajoutée */
    g_evt_tache = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!g_evt_tache) {
        DeleteCriticalSection(&g_cs);
        return -1;
    }

    g_arreter = 0;
    memset(g_file, 0, sizeof(g_file));
    g_tete = 0;
    g_queue = 0;
    g_nb = 0;
    g_id_prochain = 1;

    g_thread = CreateThread(NULL, 0, thread_llm, NULL, 0, NULL);
    if (!g_thread) {
        CloseHandle(g_evt_tache);
        g_evt_tache = NULL;
        DeleteCriticalSection(&g_cs);
        return -1;
    }

    fprintf(stderr, "[LLM-THREAD] Thread créé.\n");
    return 0;
}

void llm_thread_arreter(void) {
    if (!g_thread) return;

    /* Signaler l'arrêt */
    g_arreter = 1;
    if (g_evt_tache) SetEvent(g_evt_tache);

    /* Attendre proprement (max 5 secondes) */
    DWORD ret = WaitForSingleObject(g_thread, 5000);
    if (ret == WAIT_TIMEOUT) {
        fprintf(stderr, "[LLM-THREAD] Timeout arrêt — forçage.\n");
        TerminateThread(g_thread, 0);
    }

    CloseHandle(g_thread);
    g_thread = NULL;

    if (g_evt_tache) {
        CloseHandle(g_evt_tache);
        g_evt_tache = NULL;
    }

    DeleteCriticalSection(&g_cs);
    fprintf(stderr, "[LLM-THREAD] Thread arrêté.\n");
}

int llm_soumettre(const char* prompt, LlmCallback callback, void* donnees) {
    if (!prompt || !*prompt) return -1;

    EnterCriticalSection(&g_cs);

    if (g_nb >= LLM_TAILLE_FILE) {
        LeaveCriticalSection(&g_cs);
        fprintf(stderr, "[LLM-THREAD] File pleine, requête rejetée.\n");
        return -1;
    }

    int id = g_id_prochain++;

    LlmRequete* req = &g_file[g_queue];
    memset(req, 0, sizeof(LlmRequete));
    req->id     = id;
    req->statut = LLM_STATUT_EN_ATTENTE;
    req->callback = callback;
    req->donnees_utilisateur = donnees;
    strncpy(req->prompt, prompt, LLM_TAILLE_MAX_PROMPT - 1);
    req->prompt[LLM_TAILLE_MAX_PROMPT - 1] = '\0';

    g_queue = (g_queue + 1) % LLM_TAILLE_FILE;
    g_nb++;

    LeaveCriticalSection(&g_cs);

    /* Signaler le thread qu'une tâche est disponible */
    if (g_evt_tache) SetEvent(g_evt_tache);

    fprintf(stderr, "[LLM-THREAD] Requête #%d soumise.\n", id);
    return id;
}

LlmStatut llm_get_statut(int id_requete) {
    EnterCriticalSection(&g_cs);

    for (int i = 0; i < LLM_TAILLE_FILE; i++) {
        if (g_file[i].id == id_requete) {
            LlmStatut st = g_file[i].statut;
            LeaveCriticalSection(&g_cs);
            return st;
        }
    }

    LeaveCriticalSection(&g_cs);
    return LLM_STATUT_ERREUR; /* ID inconnu */
}

int llm_get_reponse(int id_requete, char* buffer, size_t taille_buffer) {
    if (!buffer || taille_buffer == 0) return -1;

    EnterCriticalSection(&g_cs);

    for (int i = 0; i < LLM_TAILLE_FILE; i++) {
        if (g_file[i].id == id_requete) {
            if (g_file[i].statut != LLM_STATUT_TERMINE) {
                LeaveCriticalSection(&g_cs);
                return -1;
            }
            strncpy(buffer, g_file[i].reponse, taille_buffer - 1);
            buffer[taille_buffer - 1] = '\0';
            LeaveCriticalSection(&g_cs);
            return 0;
        }
    }

    LeaveCriticalSection(&g_cs);
    return -1;
}

void llm_vider_file(void) {
    EnterCriticalSection(&g_cs);

    /* Marquer toutes les requêtes EN_ATTENTE comme annulées */
    int i = g_tete;
    int compte = g_nb;
    while (compte > 0) {
        if (g_file[i].statut == LLM_STATUT_EN_ATTENTE) {
            g_file[i].statut = LLM_STATUT_ANNULE;
        }
        i = (i + 1) % LLM_TAILLE_FILE;
        compte--;
    }

    LeaveCriticalSection(&g_cs);
}