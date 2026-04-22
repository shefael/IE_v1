#include "utils/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -----------------------------------------------------------------------
 * Fonctions utilitaires internes
 * ----------------------------------------------------------------------- */

/* Supprime les espaces en début et fin de chaîne (en place) */
static char* trim(char* s) {
    if (!s) return s;

    /* Trim gauche */
    while (*s && isspace((unsigned char)*s)) s++;

    /* Trim droit */
    char* fin = s + strlen(s);
    while (fin > s && isspace((unsigned char)*(fin - 1))) fin--;
    *fin = '\0';

    return s;
}

/* Convertit une chaîne en minuscules (en place, dans un buffer fourni) */
static void vers_minuscules(const char* src, char* dst, size_t max) {
    size_t i;
    for (i = 0; i + 1 < max && src[i]; i++) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

/* -----------------------------------------------------------------------
 * API publique
 * ----------------------------------------------------------------------- */

Config* config_charger(const char* chemin) {
    if (!chemin) return NULL;

    FILE* f = fopen(chemin, "r");
    if (!f) return NULL;

    Config* cfg = (Config*)calloc(1, sizeof(Config));
    if (!cfg) {
        fclose(f);
        return NULL;
    }

    char ligne[512];
    char section_courante[CONFIG_MAX_LEN] = "";

    while (fgets(ligne, sizeof(ligne), f)) {
        /* Supprimer le '\n' final */
        char* p = strchr(ligne, '\n');
        if (p) *p = '\0';
        p = strchr(ligne, '\r');
        if (p) *p = '\0';

        char* l = trim(ligne);

        /* Ignorer les lignes vides et les commentaires */
        if (!l || !*l || *l == ';' || *l == '#') continue;

        /* Ligne de section : [NomSection] */
        if (*l == '[') {
            char* fin_section = strchr(l, ']');
            if (fin_section) {
                *fin_section = '\0';
                char* nom = trim(l + 1);
                vers_minuscules(nom, section_courante, CONFIG_MAX_LEN);
            }
            continue;
        }

        /* Ligne clé=valeur */
        char* egal = strchr(l, '=');
        if (!egal) continue; /* Ligne malformée, on ignore */

        *egal = '\0';
        char* cle    = trim(l);
        char* valeur = trim(egal + 1);

        if (!cle || !*cle) continue;

        /* Vérifier la capacité */
        if (cfg->nb_entrees >= CONFIG_MAX_ENTREES) break;

        ConfigEntree* e = &cfg->entrees[cfg->nb_entrees];

        /* Stocker section en minuscules pour comparaison insensible */
        strncpy(e->section, section_courante, CONFIG_MAX_LEN - 1);
        e->section[CONFIG_MAX_LEN - 1] = '\0';

        /* Stocker la clé en minuscules */
        vers_minuscules(cle, e->cle, CONFIG_MAX_LEN);

        /* Stocker la valeur telle quelle (préserver la casse) */
        strncpy(e->valeur, valeur ? valeur : "", CONFIG_MAX_LEN - 1);
        e->valeur[CONFIG_MAX_LEN - 1] = '\0';

        cfg->nb_entrees++;
    }

    fclose(f);
    return cfg;
}

void config_free(Config* cfg) {
    free(cfg);
}

const char* config_get(const Config* cfg, const char* section, const char* cle) {
    if (!cfg || !section || !cle) return NULL;

    /* Normaliser section et clé en minuscules pour la comparaison */
    char section_min[CONFIG_MAX_LEN];
    char cle_min[CONFIG_MAX_LEN];
    vers_minuscules(section, section_min, CONFIG_MAX_LEN);
    vers_minuscules(cle, cle_min, CONFIG_MAX_LEN);

    for (size_t i = 0; i < cfg->nb_entrees; i++) {
        const ConfigEntree* e = &cfg->entrees[i];
        if (strcmp(e->section, section_min) == 0 &&
            strcmp(e->cle,     cle_min)     == 0) {
            return e->valeur;
        }
    }

    return NULL;
}

int config_get_int(const Config* cfg, const char* section,
                   const char* cle, int defaut) {
    const char* val = config_get(cfg, section, cle);
    if (!val || !*val) return defaut;

    char* fin;
    long resultat = strtol(val, &fin, 10);

    /* Si la conversion a échoué (fin == val), retourner defaut */
    if (fin == val) return defaut;

    return (int)resultat;
}

double config_get_double(const Config* cfg, const char* section,
                         const char* cle, double defaut) {
    const char* val = config_get(cfg, section, cle);
    if (!val || !*val) return defaut;

    char* fin;
    double resultat = strtod(val, &fin);

    if (fin == val) return defaut;

    return resultat;
}

int config_set(Config* cfg, const char* section,
               const char* cle, const char* valeur) {
    if (!cfg || !section || !cle) return -1;

    char section_min[CONFIG_MAX_LEN];
    char cle_min[CONFIG_MAX_LEN];
    vers_minuscules(section, section_min, CONFIG_MAX_LEN);
    vers_minuscules(cle, cle_min, CONFIG_MAX_LEN);

    /* Chercher si la paire existe déjà */
    for (size_t i = 0; i < cfg->nb_entrees; i++) {
        ConfigEntree* e = &cfg->entrees[i];
        if (strcmp(e->section, section_min) == 0 &&
            strcmp(e->cle,     cle_min)     == 0) {
            /* Mise à jour de la valeur existante */
            strncpy(e->valeur, valeur ? valeur : "", CONFIG_MAX_LEN - 1);
            e->valeur[CONFIG_MAX_LEN - 1] = '\0';
            return 0;
        }
    }

    /* Nouvelle entrée */
    if (cfg->nb_entrees >= CONFIG_MAX_ENTREES) return -1;

    ConfigEntree* e = &cfg->entrees[cfg->nb_entrees];
    strncpy(e->section, section_min, CONFIG_MAX_LEN - 1);
    e->section[CONFIG_MAX_LEN - 1] = '\0';
    strncpy(e->cle, cle_min, CONFIG_MAX_LEN - 1);
    e->cle[CONFIG_MAX_LEN - 1] = '\0';
    strncpy(e->valeur, valeur ? valeur : "", CONFIG_MAX_LEN - 1);
    e->valeur[CONFIG_MAX_LEN - 1] = '\0';

    cfg->nb_entrees++;
    return 0;
}

int config_sauvegarder(const Config* cfg, const char* chemin) {
    if (!cfg || !chemin) return -1;

    FILE* f = fopen(chemin, "w");
    if (!f) return -1;

    char section_courante[CONFIG_MAX_LEN] = "";

    for (size_t i = 0; i < cfg->nb_entrees; i++) {
        const ConfigEntree* e = &cfg->entrees[i];

        /* Écrire l'en-tête de section si elle change */
        if (strcmp(e->section, section_courante) != 0) {
            if (i > 0) fprintf(f, "\n"); /* Ligne vide entre sections */
            fprintf(f, "[%s]\n", e->section);
            strncpy(section_courante, e->section, CONFIG_MAX_LEN - 1);
            section_courante[CONFIG_MAX_LEN - 1] = '\0';
        }

        fprintf(f, "%s=%s\n", e->cle, e->valeur);
    }

    fclose(f);
    return 0;
}