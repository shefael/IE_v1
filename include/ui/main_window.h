#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include <windows.h>

/* -----------------------------------------------------------------------
 * IDs des menus — Fichier
 * ----------------------------------------------------------------------- */
#define ID_FICHIER_NOUVEAU      1001
#define ID_FICHIER_OUVRIR       1002
#define ID_FICHIER_ENREGISTRER  1003
#define ID_FICHIER_ENREG_SOUS   1004
#define ID_FICHIER_QUITTER      1005

/* -----------------------------------------------------------------------
 * IDs des menus — Édition
 * ----------------------------------------------------------------------- */
#define ID_EDITION_ANNULER      2001
#define ID_EDITION_REFAIRE      2002
#define ID_EDITION_COUPER       2003
#define ID_EDITION_COPIER       2004
#define ID_EDITION_COLLER       2005
#define ID_EDITION_TOUT_SELEC   2006
#define ID_EDITION_RECHERCHER   2007
#define ID_EDITION_REMPLACER    2008

/* -----------------------------------------------------------------------
 * IDs des menus — Affichage
 * ----------------------------------------------------------------------- */
#define ID_AFFICHAGE_THEME      3001

/* -----------------------------------------------------------------------
 * IDs des menus — Outils
 * ----------------------------------------------------------------------- */
#define ID_OUTILS_REGLES        4001
#define ID_OUTILS_PREFERENCES   4002
#define ID_OUTILS_REFORMULER_FORMEL    4003
#define ID_OUTILS_REFORMULER_SIMPLE    4004
#define ID_OUTILS_REFORMULER_ACADEM    4005

/* -----------------------------------------------------------------------
 * IDs des menus — Aide
 * ----------------------------------------------------------------------- */
#define ID_AIDE_APROPOS         5001

/* -----------------------------------------------------------------------
 * IDs des timers
 * ----------------------------------------------------------------------- */
#define TIMER_ORTHO_ID          101   /* Vérification orthographique (500ms)  */
#define TIMER_REGLES_ID         102   /* Rapport de conformité (2000ms)       */
#define TIMER_STATUT_ID         103   /* Mise à jour barre d'état (300ms)     */

/* -----------------------------------------------------------------------
 * Nom de la classe de fenêtre principale
 * ----------------------------------------------------------------------- */
#define CLASSE_FENETRE_PRINCIPALE  L"IntelliEditorMainWnd"

/* -----------------------------------------------------------------------
 * Handle global de la fenêtre Scintilla (accessible depuis d'autres modules)
 * ----------------------------------------------------------------------- */
extern HWND g_hwnd_scintilla;

/* -----------------------------------------------------------------------
 * Handle global de la fenêtre principale
 * ----------------------------------------------------------------------- */
extern HWND g_hwnd_principale;

/*
 * Initialise et crée la fenêtre principale.
 * Retourne TRUE en cas de succès.
 */
BOOL ui_init(HINSTANCE hInstance, int nCmdShow);

/*
 * Lance la boucle de messages Windows.
 * Bloque jusqu'à la fermeture de l'application.
 * Retourne le wParam du message WM_QUIT.
 */
int ui_run(HINSTANCE hInstance);

/*
 * Met à jour le titre de la fenêtre avec le nom du fichier courant.
 */
void ui_set_titre(const wchar_t* nom_fichier);

#endif /* UI_MAIN_WINDOW_H */