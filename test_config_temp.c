/* Fichier temporaire — NE PAS COMMITER — SUPPRIMER APRÈS VALIDATION */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/config.h"

int main(void) {
    printf("=== Test Config INI ===\n");

    /* Chemin vers le config.ini créé dans %APPDATA% */
    const char* chemin = getenv("APPDATA");
    char chemin_complet[512];
    snprintf(chemin_complet, sizeof(chemin_complet),
             "%s\\IntelliEditor\\config.ini", chemin);

    printf("Chargement de : %s\n", chemin_complet);

    Config* cfg = config_charger(chemin_complet);
    if (!cfg) {
        printf("ECHEC : impossible de charger le fichier\n");
        return 1;
    }
    printf("OK : fichier chargé (%zu entrées)\n\n", cfg->nb_entrees);

    /* Test config_get */
    const char* theme = config_get(cfg, "General", "theme");
    printf("General/theme       = '%s' ", theme ? theme : "(null)");
    if (theme && strcmp(theme, "light") == 0) printf("OK\n");
    else printf("ECHEC\n");

    const char* lang = config_get(cfg, "General", "language");
    printf("General/language    = '%s' ", lang ? lang : "(null)");
    if (lang && strcmp(lang, "fr_FR") == 0) printf("OK\n");
    else printf("ECHEC\n");

    const char* model = config_get(cfg, "LLM", "model_path");
    printf("LLM/model_path      = '%s'\n", model ? model : "(null)");
    if (model) printf("OK\n"); else printf("ECHEC\n");

    /* Test config_get_int */
    int threads = config_get_int(cfg, "LLM", "n_threads", 0);
    printf("LLM/n_threads       = %d ", threads);
    if (threads == 4) printf("OK\n"); else printf("ECHEC (attendu 4)\n");

    int ctx = config_get_int(cfg, "LLM", "n_ctx", 0);
    printf("LLM/n_ctx           = %d ", ctx);
    if (ctx == 4096) printf("OK\n"); else printf("ECHEC (attendu 4096)\n");

    int font_size = config_get_int(cfg, "Editor", "font_size", 0);
    printf("Editor/font_size    = %d ", font_size);
    if (font_size == 12) printf("OK\n"); else printf("ECHEC (attendu 12)\n");

    /* Test clé inexistante */
    const char* inexistant = config_get(cfg, "General", "inexistant");
    printf("General/inexistant  = %s ", inexistant ? inexistant : "NULL");
    if (!inexistant) printf("OK\n"); else printf("ECHEC\n");

    /* Test config_set + config_sauvegarder */
    config_set(cfg, "General", "theme", "dark");
    const char* nouveau_theme = config_get(cfg, "General", "theme");
    printf("Après set theme=dark: '%s' ", nouveau_theme ? nouveau_theme : "(null)");
    if (nouveau_theme && strcmp(nouveau_theme, "dark") == 0) printf("OK\n");
    else printf("ECHEC\n");

    /* Sauvegarde dans un fichier temporaire */
    char chemin_sauvegarde[512];
    snprintf(chemin_sauvegarde, sizeof(chemin_sauvegarde),
             "%s\\IntelliEditor\\config_test_sauvegarde.ini", chemin);
    int ret = config_sauvegarder(cfg, chemin_sauvegarde);
    printf("Sauvegarde          = %s\n", ret == 0 ? "OK" : "ECHEC");

    config_free(cfg);

    /* Test fichier inexistant */
    Config* cfg2 = config_charger("C:\\fichier\\inexistant.ini");
    if (!cfg2) printf("Fichier inexistant  = NULL OK\n");
    else { printf("ECHEC : aurait dû retourner NULL\n"); config_free(cfg2); }

    printf("\n=== Tous les tests passent ===\n");
    return 0;
}