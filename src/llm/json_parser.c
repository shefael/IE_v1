/*
 * json_parser.c — Parser des réponses JSON structurées du LLM
 *
 * Utilise cJSON pour parser les réponses.
 * Gère les cas où le LLM retourne du texte libre autour du JSON
 * via json_parser_extraire_json() qui cherche le premier '{' et
 * le dernier '}' correspondant.
 */

#include "llm/json_parser.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * json_parser_extraire_json
 *
 * Cherche le premier '{' et trouve le '}' correspondant en comptant
 * les accolades imbriquées. Retourne une copie du bloc JSON extrait.
 * -------------------------------------------------------------------------- */
char *json_parser_extraire_json(const char *texte_brut)
{
    if (!texte_brut) return NULL;

    /* Chercher le premier '{' */
    const char *debut = strchr(texte_brut, '{');
    if (!debut) return NULL;

    /* Compter les accolades pour trouver la fermeture */
    int  profondeur = 0;
    int  dans_chaine = 0;
    char precedent   = 0;
    const char *p    = debut;
    const char *fin  = NULL;

    while (*p) {
        char c = *p;

        /* Gestion des chaînes JSON (ignorer les accolades dans les strings) */
        if (c == '"' && precedent != '\\') {
            dans_chaine = !dans_chaine;
        }

        if (!dans_chaine) {
            if (c == '{') profondeur++;
            else if (c == '}') {
                profondeur--;
                if (profondeur == 0) {
                    fin = p;
                    break;
                }
            }
        }

        precedent = c;
        p++;
    }

    if (!fin) return NULL;

    size_t longueur = (size_t)(fin - debut + 1);
    char  *json     = (char *)malloc(longueur + 1);
    if (!json) return NULL;

    memcpy(json, debut, longueur);
    json[longueur] = '\0';
    return json;
}

/* --------------------------------------------------------------------------
 * json_parser_erreurs_grammaire
 * -------------------------------------------------------------------------- */
ErreurGrammaire *json_parser_erreurs_grammaire(const char *json_brut,
                                                int *nb_erreurs)
{
    if (!nb_erreurs) return NULL;
    *nb_erreurs = 0;

    if (!json_brut) return NULL;

    /* Extraire le JSON depuis le texte brut */
    char *json_net = json_parser_extraire_json(json_brut);
    if (!json_net) {
        fprintf(stderr, "[json_parser] Aucun JSON valide dans la réponse\n");
        return NULL;
    }

    cJSON *root = cJSON_Parse(json_net);
    free(json_net);

    if (!root) {
        fprintf(stderr, "[json_parser] Échec parsing JSON grammaire\n");
        return NULL;
    }

    /* Extraire le tableau "errors" */
    cJSON *errors = cJSON_GetObjectItemCaseSensitive(root, "errors");
    if (!cJSON_IsArray(errors)) {
        cJSON_Delete(root);
        return NULL;
    }

    int nb = cJSON_GetArraySize(errors);
    if (nb <= 0) {
        cJSON_Delete(root);
        *nb_erreurs = 0;
        return NULL;
    }

    ErreurGrammaire *resultat = (ErreurGrammaire *)calloc(
        (size_t)nb, sizeof(ErreurGrammaire));
    if (!resultat) {
        cJSON_Delete(root);
        return NULL;
    }

    int nb_valides = 0;
    for (int i = 0; i < nb; i++) {
        cJSON *item = cJSON_GetArrayItem(errors, i);
        if (!item) continue;

        cJSON *type        = cJSON_GetObjectItem(item, "type");
        cJSON *original    = cJSON_GetObjectItem(item, "original");
        cJSON *corrected   = cJSON_GetObjectItem(item, "corrected");
        cJSON *explanation = cJSON_GetObjectItem(item, "explanation");

        ErreurGrammaire *e = &resultat[nb_valides];
        e->type        = cJSON_IsString(type)
                         ? _strdup(type->valuestring) : _strdup("unknown");
        e->original    = cJSON_IsString(original)
                         ? _strdup(original->valuestring) : _strdup("");
        e->corrected   = cJSON_IsString(corrected)
                         ? _strdup(corrected->valuestring) : _strdup("");
        e->explanation = cJSON_IsString(explanation)
                         ? _strdup(explanation->valuestring) : _strdup("");
        nb_valides++;
    }

    cJSON_Delete(root);
    *nb_erreurs = nb_valides;

    if (nb_valides == 0) {
        free(resultat);
        return NULL;
    }

    return resultat;
}

/* --------------------------------------------------------------------------
 * json_parser_reformulation
 * -------------------------------------------------------------------------- */
ResultatReformulation *json_parser_reformulation(const char *json_brut)
{
    if (!json_brut) return NULL;

    char *json_net = json_parser_extraire_json(json_brut);
    if (!json_net) {
        fprintf(stderr, "[json_parser] Aucun JSON valide (reformulation)\n");
        return NULL;
    }

    cJSON *root = cJSON_Parse(json_net);
    free(json_net);

    if (!root) {
        fprintf(stderr, "[json_parser] Échec parsing JSON reformulation\n");
        return NULL;
    }

    cJSON *reformulated = cJSON_GetObjectItem(root, "reformulated");
    cJSON *explanation  = cJSON_GetObjectItem(root, "explanation");

    if (!cJSON_IsString(reformulated)) {
        cJSON_Delete(root);
        return NULL;
    }

    ResultatReformulation *res = (ResultatReformulation *)calloc(
        1, sizeof(ResultatReformulation));
    if (!res) {
        cJSON_Delete(root);
        return NULL;
    }

    res->reformule   = _strdup(reformulated->valuestring);
    res->explication = cJSON_IsString(explanation)
                       ? _strdup(explanation->valuestring)
                       : _strdup("");

    cJSON_Delete(root);
    return res;
}

/* --------------------------------------------------------------------------
 * json_parser_semantique
 * -------------------------------------------------------------------------- */
ResultatSemantique *json_parser_semantique(const char *json_brut)
{
    if (!json_brut) return NULL;

    char *json_net = json_parser_extraire_json(json_brut);
    if (!json_net) {
        fprintf(stderr, "[json_parser] Aucun JSON valide (sémantique)\n");
        return NULL;
    }

    cJSON *root = cJSON_Parse(json_net);
    free(json_net);

    if (!root) {
        fprintf(stderr, "[json_parser] Échec parsing JSON sémantique\n");
        return NULL;
    }

    cJSON *reponse       = cJSON_GetObjectItem(root, "reponse");
    cJSON *confiance     = cJSON_GetObjectItem(root, "confiance");
    cJSON *justification = cJSON_GetObjectItem(root, "justification");

    ResultatSemantique *res = (ResultatSemantique *)calloc(
        1, sizeof(ResultatSemantique));
    if (!res) {
        cJSON_Delete(root);
        return NULL;
    }

    /* "reponse" peut être bool ou string "true"/"false" */
    if (cJSON_IsBool(reponse)) {
        res->reponse = cJSON_IsTrue(reponse) ? 1 : 0;
    } else if (cJSON_IsString(reponse)) {
        res->reponse = (strcmp(reponse->valuestring, "true") == 0) ? 1 : 0;
    } else {
        res->reponse = 0;
    }

    res->confiance = cJSON_IsNumber(confiance)
                     ? confiance->valuedouble : 0.5;

    res->justification = cJSON_IsString(justification)
                         ? _strdup(justification->valuestring)
                         : _strdup("Aucune justification fournie");

    cJSON_Delete(root);
    return res;
}

/* --------------------------------------------------------------------------
 * Fonctions de libération
 * -------------------------------------------------------------------------- */
void json_parser_liberer_erreurs(ErreurGrammaire *erreurs, int nb)
{
    if (!erreurs) return;
    for (int i = 0; i < nb; i++) {
        free(erreurs[i].type);
        free(erreurs[i].original);
        free(erreurs[i].corrected);
        free(erreurs[i].explanation);
    }
    free(erreurs);
}

void json_parser_liberer_reformulation(ResultatReformulation *res)
{
    if (!res) return;
    free(res->reformule);
    free(res->explication);
    free(res);
}

void json_parser_liberer_semantique(ResultatSemantique *res)
{
    if (!res) return;
    free(res->justification);
    free(res);
}