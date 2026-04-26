#ifndef UI_TOOLBAR_H
#define UI_TOOLBAR_H

/*
 * toolbar.h — Barre d'outils principale
 *
 * Utilise la classe Windows ToolbarWindow32 (commctrl).
 * Chaque bouton envoie un WM_COMMAND avec son ID vers la fenêtre parente.
 */

#include <windows.h>

/* --------------------------------------------------------------------------
 * IDs des boutons de la barre d'outils
 * Ces IDs correspondent aux IDs de menu définis dans main_window.h
 * pour que les handlers WM_COMMAND existants les traitent automatiquement.
 * -------------------------------------------------------------------------- */
#define ID_TB_NOUVEAU       1001  /* Même que ID_FICHIER_NOUVEAU */
#define ID_TB_OUVRIR        1002  /* Même que ID_FICHIER_OUVRIR  */
#define ID_TB_ENREGISTRER   1003  /* Même que ID_FICHIER_ENREGISTRER */
#define ID_TB_GRAS          3101
#define ID_TB_ITALIQUE      3102
#define ID_TB_SOULIGNE      3103
#define ID_TB_TITRE1        3104
#define ID_TB_TITRE2        3105
#define ID_TB_TITRE3        3106
/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Crée la barre d'outils enfant de `parent`.
 * Retourne le HWND de la toolbar, NULL en cas d'échec.
 */
HWND toolbar_creer(HWND parent);

/*
 * Retourne la hauteur de la barre d'outils (pour le calcul de layout).
 */
int toolbar_get_hauteur(HWND hwnd_tb);

#endif /* UI_TOOLBAR_H */