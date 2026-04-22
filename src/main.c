#include "ui/main_window.h"
#include "utils/config.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * Point d'entrée Windows.
 * WinMain remplace main() pour les applications GUI Win32.
 */
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    /* Charger la configuration depuis %APPDATA%\IntelliEditor\config.ini */
    char chemin_config[MAX_PATH];
    const char* appdata = getenv("APPDATA");
    if (appdata) {
        snprintf(chemin_config, MAX_PATH,
                 "%s\\IntelliEditor\\config.ini", appdata);
    } else {
        strncpy(chemin_config, "config.ini", MAX_PATH - 1);
    }

    Config* cfg = config_charger(chemin_config);
    /* Si le fichier n'existe pas encore, cfg sera NULL — pas bloquant */

    /* Initialiser et afficher la fenêtre principale */
    if (!ui_init(hInstance, nCmdShow)) {
        config_free(cfg);
        return 1;
    }

    /* Lancer la boucle de messages (bloquant jusqu'à fermeture) */
    int code_retour = ui_run(hInstance);

    config_free(cfg);
    return code_retour;
}