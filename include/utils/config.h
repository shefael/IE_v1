#ifndef UTILS_CONFIG_H
#define UTILS_CONFIG_H

#include <stddef.h>

/*
 * Parser INI minimaliste.
 *
 * Format supporté :
 *   [Section]
 *   cle=valeur
 *   ; commentaire (ignoré)
 *   # commentaire (ignoré)
 *
 * Les espaces autour de la clé et de la valeur sont supprimés.
 * Les sections et clés sont insensibles à la casse (comparaison lowercase).
 */

/* Nombre maximum de paires clé/valeur */
#define CONFIG_MAX_ENTREES 128

/* Longueur maximale d'une section, clé ou valeur */
#define CONFIG_MAX_LEN 256

typedef struct {
    char section[CONFIG_MAX_LEN];
    char cle    [CONFIG_MAX_LEN];
    char valeur [CONFIG_MAX_LEN];
} ConfigEntree;

typedef struct {
    ConfigEntree entrees[CONFIG_MAX_ENTREES];
    size_t       nb_entrees;
} Config;

/*
 * Charge et parse un fichier INI depuis 'chemin'.
 * Alloue et retourne une Config, ou NULL en cas d'erreur.
 * L'appelant doit libérer avec config_free().
 */
Config* config_charger(const char* chemin);

/*
 * Libère la mémoire d'une Config.
 */
void config_free(Config* cfg);

/*
 * Retourne la valeur associée à [section] / cle.
 * Retourne NULL si la paire n'existe pas.
 * La chaîne retournée appartient à la Config — ne pas libérer.
 */
const char* config_get(const Config* cfg, const char* section, const char* cle);

/*
 * Retourne la valeur sous forme d'entier.
 * Retourne 'defaut' si la clé n'existe pas ou si la valeur n'est pas un entier.
 */
int config_get_int(const Config* cfg, const char* section,
                   const char* cle, int defaut);

/*
 * Retourne la valeur sous forme de flottant.
 * Retourne 'defaut' si la clé n'existe pas.
 */
double config_get_double(const Config* cfg, const char* section,
                         const char* cle, double defaut);

/*
 * Écrit ou met à jour une valeur dans la Config en mémoire.
 * Ne modifie pas le fichier sur disque (utiliser config_sauvegarder).
 * Retourne 0 en cas de succès, -1 si la Config est pleine.
 */
int config_set(Config* cfg, const char* section,
               const char* cle, const char* valeur);

/*
 * Sauvegarde la Config dans un fichier INI.
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int config_sauvegarder(const Config* cfg, const char* chemin);

#endif /* UTILS_CONFIG_H */