/*
 * rules_panel.c — Panneau latéral de conformité des règles
 *
 * Structure de la fenêtre :
 *   - Une fenêtre enfant (HWND panel) contenant :
 *     - Un label statique "Règles de rédaction" en haut
 *     - Un SysListView32 en mode Report occupant le reste
 *
 * Colonnes du ListView :
 *   [0] Statut   (40px)
 *   [1] ID       (50px)
 *   [2] Description (reste)
 */

#include "ui/rules_panel.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

/* Identifiant interne du ListView dans le panneau */
#define ID_LISTVIEW_REGLES  3001
#define HAUTEUR_LABEL       24

/* Variable statique pour accéder au ListView depuis les fonctions */
static HWND g_hwnd_listview = NULL;

/* Données des éléments courants (pour mise à jour unitaire) */
#define MAX_ELEMENTS_PANEL 64
static ElementRegle g_elements[MAX_ELEMENTS_PANEL];
static int          g_nb_elements = 0;

/* --------------------------------------------------------------------------
 * Utilitaires internes
 * -------------------------------------------------------------------------- */

/*
 * Convertit une chaîne UTF-8 en UTF-16 dans un buffer statique.
 * Attention : utiliser immédiatement, le buffer est écrasé à chaque appel.
 */
static const wchar_t *utf8_vers_w(const char *src)
{
    static wchar_t buf[512];
    if (!src || src[0] == '\0') { buf[0] = L'\0'; return buf; }
    MultiByteToWideChar(CP_UTF8, 0, src, -1, buf, 512);
    return buf;
}

/*
 * Insère une colonne dans le ListView.
 */
static void lv_ajouter_colonne(HWND hwnd_lv, int index,
                                const wchar_t *titre, int largeur)
{
    LVCOLUMNW col = {0};
    col.mask    = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = (LPWSTR)titre;
    col.cx      = largeur;
    col.iSubItem = index;
    SendMessageW(hwnd_lv, LVM_INSERTCOLUMNW, (WPARAM)index, (LPARAM)&col);
}

/*
 * Insère une ligne dans le ListView.
 */
static void lv_ajouter_ligne(HWND hwnd_lv, int index,
                              const wchar_t *col0,
                              const wchar_t *col1,
                              const wchar_t *col2)
{
    LVITEMW item = {0};
    item.mask    = LVIF_TEXT;
    item.iItem   = index;
    item.iSubItem = 0;
    item.pszText = (LPWSTR)col0;
    SendMessageW(hwnd_lv, LVM_INSERTITEMW, 0, (LPARAM)&item);

    /* Sous-éléments */
    item.iSubItem = 1;
    item.pszText  = (LPWSTR)col1;
    SendMessageW(hwnd_lv, LVM_SETITEMTEXTW, (WPARAM)index, (LPARAM)&item);

    item.iSubItem = 2;
    item.pszText  = (LPWSTR)col2;
    SendMessageW(hwnd_lv, LVM_SETITEMTEXTW, (WPARAM)index, (LPARAM)&item);
}

/*
 * Met à jour le texte d'une cellule existante.
 */
static void lv_maj_cellule(HWND hwnd_lv, int ligne, int colonne,
                            const wchar_t *texte)
{
    LVITEMW item = {0};
    item.mask     = LVIF_TEXT;
    item.iSubItem = colonne;
    item.pszText  = (LPWSTR)texte;
    SendMessageW(hwnd_lv, LVM_SETITEMTEXTW, (WPARAM)ligne, (LPARAM)&item);
}

/* --------------------------------------------------------------------------
 * rules_panel_statut_texte
 * -------------------------------------------------------------------------- */
const wchar_t *rules_panel_statut_texte(RuleStatus statut)
{
    switch (statut) {
        case STATUS_OK:        return L"[OK]";
        case STATUS_WARNING:   return L"[!] ";
        case STATUS_VIOLATION: return L"[X] ";
        case RULE_STATUS_PENDING:   return L"[..] ";
        default:               return L"[?] ";
    }
}

/* --------------------------------------------------------------------------
 * Procédure de la fenêtre panneau (sous-fenêtre conteneur)
 * -------------------------------------------------------------------------- */
static LRESULT CALLBACK panel_proc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_SIZE: {
            /* Redimensionner le ListView pour occuper tout l'espace
             * sous le label */
            RECT rc;
            GetClientRect(hwnd, &rc);
            HWND hwnd_lv = GetDlgItem(hwnd, ID_LISTVIEW_REGLES);
            if (hwnd_lv) {
                SetWindowPos(hwnd_lv, NULL,
                             0, HAUTEUR_LABEL,
                             rc.right,
                             rc.bottom - HAUTEUR_LABEL,
                             SWP_NOZORDER | SWP_NOACTIVATE);

                /* Redimensionner la colonne Description pour remplir */
                int largeur_totale = rc.right;
                int largeur_col2   = largeur_totale - 40 - 55;
                if (largeur_col2 < 60) largeur_col2 = 60;
                SendMessage(hwnd_lv, LVM_SETCOLUMNWIDTH,
                            2, MAKELPARAM(largeur_col2, 0));
            }
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            /* Fond blanc pour le label */
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, RGB(245, 245, 245));
            SetTextColor(hdc, RGB(50, 50, 50));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

/* --------------------------------------------------------------------------
 * rules_panel_creer
 * -------------------------------------------------------------------------- */
HWND rules_panel_creer(HWND parent, int x, int y,
                        int largeur, int hauteur)
{
    if (!parent) return NULL;

    /* Enregistrer la classe du panneau conteneur */
    static BOOL classe_enregistree = FALSE;
    if (!classe_enregistree) {
        WNDCLASSEXW wc = {0};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = panel_proc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.hbrBackground = CreateSolidBrush(RGB(245, 245, 245));
        wc.lpszClassName = L"IntelliEditorRulesPanel";
        RegisterClassExW(&wc);
        classe_enregistree = TRUE;
    }

    /* Créer la fenêtre conteneur */
    HWND hwnd_panel = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"IntelliEditorRulesPanel",
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, largeur, hauteur,
        parent, NULL,
        GetModuleHandle(NULL), NULL
    );
    if (!hwnd_panel) return NULL;

    /* Label titre */
    CreateWindowExW(
        0, L"STATIC",
        L"  Règles de rédaction",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, largeur, HAUTEUR_LABEL,
        hwnd_panel,
        NULL,
        GetModuleHandle(NULL), NULL
    );

    /* ListView en mode Report */
    HWND hwnd_lv = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWW,
        NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
        LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
        0, HAUTEUR_LABEL,
        largeur, hauteur - HAUTEUR_LABEL,
        hwnd_panel,
        (HMENU)(INT_PTR)ID_LISTVIEW_REGLES,
        GetModuleHandle(NULL), NULL
    );

    if (!hwnd_lv) return hwnd_panel;

    /* Style étendu ListView : lignes alternées + sélection ligne entière */
    SendMessage(hwnd_lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    /* Colonnes */
    lv_ajouter_colonne(hwnd_lv, 0, L"Statut",      42);
    lv_ajouter_colonne(hwnd_lv, 1, L"ID",           50);
    lv_ajouter_colonne(hwnd_lv, 2, L"Description", 140);

    g_hwnd_listview = hwnd_lv;

    /* Message initial */
    rules_panel_vider(hwnd_panel);

    return hwnd_panel;
}

/* --------------------------------------------------------------------------
 * rules_panel_redimensionner
 * -------------------------------------------------------------------------- */
void rules_panel_redimensionner(HWND hwnd_panel, int x, int y,
                                 int largeur, int hauteur)
{
    if (!hwnd_panel) return;
    SetWindowPos(hwnd_panel, NULL, x, y, largeur, hauteur,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

/* --------------------------------------------------------------------------
 * rules_panel_peupler
 * -------------------------------------------------------------------------- */
void rules_panel_peupler(HWND hwnd_panel,
                          const ElementRegle *elements,
                          int nb_elements)
{
    if (!hwnd_panel || !g_hwnd_listview) return;

    /* Vider la liste */
    SendMessage(g_hwnd_listview, LVM_DELETEALLITEMS, 0, 0);
    g_nb_elements = 0;

    if (!elements || nb_elements <= 0) {
        rules_panel_vider(hwnd_panel);
        return;
    }

    int nb = nb_elements < MAX_ELEMENTS_PANEL
             ? nb_elements : MAX_ELEMENTS_PANEL;

    for (int i = 0; i < nb; i++) {
        /* Copier dans le tableau interne */
        g_elements[i] = elements[i];

        const wchar_t *statut_w = rules_panel_statut_texte(elements[i].statut);
        const wchar_t *id_w     = utf8_vers_w(elements[i].id);

        /* Description : si message disponible, l'afficher après la description */
        char desc_complete[512];
        if (elements[i].message[0] != '\0') {
            snprintf(desc_complete, sizeof(desc_complete),
                     "%s — %s",
                     elements[i].description,
                     elements[i].message);
        } else {
            snprintf(desc_complete, sizeof(desc_complete),
                     "%s", elements[i].description);
        }

        lv_ajouter_ligne(g_hwnd_listview, i,
                          statut_w,
                          id_w,
                          utf8_vers_w(desc_complete));
    }

    g_nb_elements = nb;
}

/* --------------------------------------------------------------------------
 * rules_panel_maj_statut
 * Mise à jour unitaire d'une règle sans repeupler toute la liste.
 * -------------------------------------------------------------------------- */
void rules_panel_maj_statut(HWND hwnd_panel, const char *rule_id,
                             RuleStatus statut, const char *message)
{
    if (!hwnd_panel || !g_hwnd_listview || !rule_id) return;

    /* Trouver l'index de la règle dans le tableau interne */
    for (int i = 0; i < g_nb_elements; i++) {
        if (strcmp(g_elements[i].id, rule_id) == 0) {
            /* Mettre à jour le tableau interne */
            g_elements[i].statut = statut;
            if (message) {
                strncpy(g_elements[i].message, message, 255);
                g_elements[i].message[255] = '\0';
            }

            /* Mettre à jour la cellule statut */
            lv_maj_cellule(g_hwnd_listview, i, 0,
                           rules_panel_statut_texte(statut));

            /* Mettre à jour la description avec le message */
            if (message && message[0] != '\0') {
                char desc_complete[512];
                snprintf(desc_complete, sizeof(desc_complete),
                         "%s — %s",
                         g_elements[i].description, message);
                lv_maj_cellule(g_hwnd_listview, i, 2,
                               utf8_vers_w(desc_complete));
            }
            return;
        }
    }
}

/* --------------------------------------------------------------------------
 * rules_panel_vider
 * -------------------------------------------------------------------------- */
void rules_panel_vider(HWND hwnd_panel)
{
    if (!g_hwnd_listview) return;

    SendMessage(g_hwnd_listview, LVM_DELETEALLITEMS, 0, 0);
    g_nb_elements = 0;

    /* Insérer un message informatif */
    lv_ajouter_ligne(g_hwnd_listview, 0,
                     L"",
                     L"",
                     L"Charger un fichier de règles");
    lv_ajouter_ligne(g_hwnd_listview, 1,
                     L"",
                     L"",
                     L"via Outils > Charger les règles");
}