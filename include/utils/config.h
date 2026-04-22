#ifndef UTILS_CONFIG_H
#define UTILS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Charge la configuration depuis un fichier JSON */
void* config_load(const char* path);

/* Sauvegarde la configuration dans un fichier JSON */
int config_save(const char* path, const void* config);

/* Libère la mémoire associée à la configuration */
void config_free(void* config);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_CONFIG_H */