/*
 * toolbar.c — Implémentation de la barre d'outils
 *
 * On utilise la classe ToolbarWindow32 de commctrl.
 * Les boutons sont définis avec des labels texte (pas d'icônes bitmap)
 * pour éviter les dépendances sur des ressources externes.
 */

#include "ui/toolbar.h"
#include "ui/main_window.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

/* Hauteur fixe de la barre d'outils */
#define HAUTEUR_TOOLBAR 28

/*
 * Structure décrivant un bouton de la barre d'outils.
 * On utilise TBBUTTON avec le style BTNS_AUTOSIZE pour que
 * chaque bouton s'adapte à la largeur de son texte.
 */
typedef struct {
    int         id;       /* ID de commande WM_COMMAND */
    const char *label;    /* Texte affiché sur le bouton */
    BYTE        style;    /* BTNS_BUTTON, BTNS_SEP */
} DefinitionBouton;

/* --------------------------------------------------------------------------
 * Définition des boutons
 * BTNS_SEP = séparateur (id et label ignorés)
 * -------------------------------------------------------------------------- */
static const DefinitionBouton g_boutons[] = {
    { ID_FICHIER_NOUVEAU,      "Nouveau",  BTNS_BUTTON   },
    { ID_FICHIER_OUVRIR,       "Ouvrir",   BTNS_BUTTON   },
    { ID_FICHIER_ENREGISTRER,  "Enreg.",   BTNS_BUTTON   },
    { 0,                       NULL,       BTNS_SEP      },
    { ID_TB_GRAS,              "G",        BTNS_BUTTON   },
    { ID_TB_ITALIQUE,          "I",        BTNS_BUTTON   },
    { ID_TB_SOULIGNE,          "S",        BTNS_BUTTON   },
    { 0,                       NULL,       BTNS_SEP      },
    { ID_TB_TITRE1,            "H1",       BTNS_BUTTON   },
    { ID_TB_TITRE2,            "H2",       BTNS_BUTTON   },
    { ID_TB_TITRE3,            "H3",       BTNS_BUTTON   },
};

#define NB_BOUTONS ((int)(sizeof(g_boutons) / sizeof(g_boutons[0])))

/* --------------------------------------------------------------------------
 * toolbar_creer
 * -------------------------------------------------------------------------- */
HWND toolbar_creer(HWND parent)
{
    if (!parent) return NULL;

    /*
     * Créer la fenêtre toolbar avec le style standard Windows.
     * CCS_NODIVIDER supprime la ligne de séparation en haut.
     * TBSTYLE_FLAT    : boutons plats (style moderne).
     * TBSTYLE_TOOLTIPS: info-bulles automatiques.
     */
    HWND hwnd_tb = CreateWindowExW(
        0,
        TOOLBARCLASSNAMEW,
        NULL,
        WS_CHILD | WS_VISIBLE | CCS_NODIVIDER |
        TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
        0, 0, 0, HAUTEUR_TOOLBAR,
        parent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!hwnd_tb) return NULL;

    /* Indispensable : envoyer TB_BUTTONSTRUCTSIZE avant tout */
    SendMessage(hwnd_tb, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    /*
     * On utilise une ImageList vide (pas d'icônes) pour éviter
     * l'affichage d'icônes par défaut. Les boutons sont en texte pur.
     */
    SendMessage(hwnd_tb, TB_SETIMAGELIST, 0, (LPARAM)NULL);

    /*
     * Construire les TBBUTTON.
     * Pour les boutons texte, on utilise une string pool :
     * TB_ADDSTRING ajoute les chaînes et retourne l'index.
     * Chaque chaîne est terminée par \0\0 (double null).
     */
    TBBUTTON boutons[NB_BOUTONS];
    memset(boutons, 0, sizeof(boutons));

    for (int i = 0; i < NB_BOUTONS; i++) {
        const DefinitionBouton *def = &g_boutons[i];

        if (def->style == BTNS_SEP) {
            boutons[i].fsStyle = BTNS_SEP;
            boutons[i].iBitmap = 6; /* Largeur du séparateur en pixels */
            continue;
        }

        /* Ajouter la chaîne dans la string pool de la toolbar */
        wchar_t label_w[64];
        MultiByteToWideChar(CP_UTF8, 0, def->label, -1,
                            label_w, 64);

        INT_PTR idx_str = SendMessageW(hwnd_tb, TB_ADDSTRINGW,
                                        0, (LPARAM)label_w);

        boutons[i].iBitmap   = I_IMAGENONE; /* Pas d'icône */
        boutons[i].idCommand = def->id;
        boutons[i].fsState   = TBSTATE_ENABLED;
        boutons[i].fsStyle   = def->style | BTNS_AUTOSIZE;
        boutons[i].iString   = idx_str;
    }

    /* Ajouter les boutons */
    SendMessage(hwnd_tb, TB_ADDBUTTONS, (WPARAM)NB_BOUTONS,
                (LPARAM)boutons);

    /* Recalcule la taille des boutons */
    SendMessage(hwnd_tb, TB_AUTOSIZE, 0, 0);

    return hwnd_tb;
}

/* --------------------------------------------------------------------------
 * toolbar_get_hauteur
 * -------------------------------------------------------------------------- */
int toolbar_get_hauteur(HWND hwnd_tb)
{
    if (!hwnd_tb) return HAUTEUR_TOOLBAR;

    RECT rect;
    if (GetWindowRect(hwnd_tb, &rect)) {
        return rect.bottom - rect.top;
    }
    return HAUTEUR_TOOLBAR;
}