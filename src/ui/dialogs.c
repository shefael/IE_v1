/*
 * dialogs.c — Boîtes de dialogue natives Windows
 *
 * Utilise GetOpenFileNameW / GetSaveFileNameW pour les fichiers.
 * Le dialogue Rechercher/Remplacer est créé manuellement avec
 * des contrôles Win32 (pas de ressource .rc).
 *
 * Tous les chemins retournés sont en UTF-8 (conversion depuis UTF-16).
 */

#include "ui/dialogs.h"
#include "utils/encoding.h"

#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Utilitaire : convertit un chemin UTF-16 en UTF-8 alloué dynamiquement
 * -------------------------------------------------------------------------- */
static char *chemin_w_vers_utf8(const wchar_t *chemin_w)
{
    if (!chemin_w || chemin_w[0] == L'\0') return NULL;

    int taille = WideCharToMultiByte(CP_UTF8, 0, chemin_w, -1,
                                      NULL, 0, NULL, NULL);
    if (taille <= 0) return NULL;

    char *chemin = (char *)malloc((size_t)taille);
    if (!chemin) return NULL;

    WideCharToMultiByte(CP_UTF8, 0, chemin_w, -1,
                        chemin, taille, NULL, NULL);
    return chemin;
}

/* --------------------------------------------------------------------------
 * dialog_ouvrir_fichier
 * -------------------------------------------------------------------------- */
char *dialog_ouvrir_fichier(HWND parent)
{
    wchar_t chemin_w[MAX_PATH] = {0};

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = parent;
    ofn.lpstrFilter  =
        L"Tous les formats\0*.txt;*.rtf;*.ie\0"
        L"Texte brut (*.txt)\0*.txt\0"
        L"Rich Text (*.rtf)\0*.rtf\0"
        L"IntelliEditor (*.ie)\0*.ie\0"
        L"Tous les fichiers (*.*)\0*.*\0";
    ofn.lpstrFile    = chemin_w;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = L"Ouvrir un document";
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                       OFN_HIDEREADONLY;
    ofn.lpstrDefExt  = L"txt";

    if (!GetOpenFileNameW(&ofn)) return NULL;
    return chemin_w_vers_utf8(chemin_w);
}

/* --------------------------------------------------------------------------
 * dialog_enregistrer_fichier
 * -------------------------------------------------------------------------- */
char *dialog_enregistrer_fichier(HWND parent)
{
    wchar_t chemin_w[MAX_PATH] = {0};

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = parent;
    ofn.lpstrFilter  =
        L"Texte brut (*.txt)\0*.txt\0"
        L"Rich Text (*.rtf)\0*.rtf\0"
        L"IntelliEditor (*.ie)\0*.ie\0";
    ofn.lpstrFile    = chemin_w;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = L"Enregistrer sous";
    ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt  = L"txt";

    if (!GetSaveFileNameW(&ofn)) return NULL;
    return chemin_w_vers_utf8(chemin_w);
}

/* --------------------------------------------------------------------------
 * dialog_ouvrir_regles
 * -------------------------------------------------------------------------- */
char *dialog_ouvrir_regles(HWND parent)
{
    wchar_t chemin_w[MAX_PATH] = {0};

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = parent;
    ofn.lpstrFilter  =
        L"Fichiers de règles (*.json)\0*.json\0"
        L"Tous les fichiers (*.*)\0*.*\0";
    ofn.lpstrFile    = chemin_w;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = L"Charger un fichier de règles";
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                       OFN_HIDEREADONLY;
    ofn.lpstrDefExt  = L"json";

    if (!GetOpenFileNameW(&ofn)) return NULL;
    return chemin_w_vers_utf8(chemin_w);
}

/* --------------------------------------------------------------------------
 * Dialogue Rechercher/Remplacer — Données internes
 * -------------------------------------------------------------------------- */
#define ID_EDIT_RECHERCHE    4001
#define ID_EDIT_REMPLACE     4002
#define ID_CHK_REGEX         4003
#define ID_CHK_CASSE         4004
#define ID_BTN_OK            4005
#define ID_BTN_ANNULER       4006

typedef struct {
    char  recherche[512];
    char  remplacement[512];
    int   utiliser_regex;
    int   sensible_casse;
    int   valide;           /* 1 si OK cliqué */
} DonneesDialogRecherche;

static DonneesDialogRecherche g_donnees_recherche;

static LRESULT CALLBACK recherche_proc(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE: {
            int y = 10;

            /* Label + champ Rechercher */
            CreateWindowExW(0, L"STATIC", L"Rechercher :",
                WS_CHILD | WS_VISIBLE,
                10, y, 100, 20, hwnd, NULL,
                GetModuleHandle(NULL), NULL);
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                120, y, 250, 22, hwnd,
                (HMENU)(INT_PTR)ID_EDIT_RECHERCHE,
                GetModuleHandle(NULL), NULL);
            y += 32;

            /* Label + champ Remplacer */
            CreateWindowExW(0, L"STATIC", L"Remplacer par :",
                WS_CHILD | WS_VISIBLE,
                10, y, 100, 20, hwnd, NULL,
                GetModuleHandle(NULL), NULL);
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                120, y, 250, 22, hwnd,
                (HMENU)(INT_PTR)ID_EDIT_REMPLACE,
                GetModuleHandle(NULL), NULL);
            y += 36;

            /* Cases à cocher */
            CreateWindowExW(0, L"BUTTON", L"Expression régulière (PCRE2)",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                10, y, 240, 20, hwnd,
                (HMENU)(INT_PTR)ID_CHK_REGEX,
                GetModuleHandle(NULL), NULL);
            y += 26;

            CreateWindowExW(0, L"BUTTON", L"Respecter la casse",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                10, y, 200, 20, hwnd,
                (HMENU)(INT_PTR)ID_CHK_CASSE,
                GetModuleHandle(NULL), NULL);
            /* Casse activée par défaut */
            SendDlgItemMessageW(hwnd, ID_CHK_CASSE, BM_SETCHECK,
                                BST_CHECKED, 0);
            y += 36;

            /* Boutons */
            CreateWindowExW(0, L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                120, y, 80, 28, hwnd,
                (HMENU)(INT_PTR)ID_BTN_OK,
                GetModuleHandle(NULL), NULL);
            CreateWindowExW(0, L"BUTTON", L"Annuler",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                210, y, 80, 28, hwnd,
                (HMENU)(INT_PTR)ID_BTN_ANNULER,
                GetModuleHandle(NULL), NULL);
            return 0;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);

            if (id == ID_BTN_OK || id == IDOK) {
                /* Récupérer les valeurs des champs */
                wchar_t buf_w[512];

                GetDlgItemTextW(hwnd, ID_EDIT_RECHERCHE, buf_w, 512);
                WideCharToMultiByte(CP_UTF8, 0, buf_w, -1,
                    g_donnees_recherche.recherche, 512, NULL, NULL);

                GetDlgItemTextW(hwnd, ID_EDIT_REMPLACE, buf_w, 512);
                WideCharToMultiByte(CP_UTF8, 0, buf_w, -1,
                    g_donnees_recherche.remplacement, 512, NULL, NULL);

                g_donnees_recherche.utiliser_regex =
                    (SendDlgItemMessageW(hwnd, ID_CHK_REGEX,
                                         BM_GETCHECK, 0, 0) == BST_CHECKED)
                    ? 1 : 0;

                g_donnees_recherche.sensible_casse =
                    (SendDlgItemMessageW(hwnd, ID_CHK_CASSE,
                                         BM_GETCHECK, 0, 0) == BST_CHECKED)
                    ? 1 : 0;

                g_donnees_recherche.valide = 1;
                DestroyWindow(hwnd);
                return 0;
            }

            if (id == ID_BTN_ANNULER || id == IDCANCEL) {
                g_donnees_recherche.valide = 0;
                DestroyWindow(hwnd);
                return 0;
            }
            return 0;
        }

        case WM_CLOSE:
            g_donnees_recherche.valide = 0;
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

/* --------------------------------------------------------------------------
 * dialog_rechercher_remplacer
 * -------------------------------------------------------------------------- */
int dialog_rechercher_remplacer(HWND parent,
                                 ContexteRecherche **out_ctx,
                                 char *out_remplacement,
                                 int taille_remplacement)
{
    if (!out_ctx) return 0;
    *out_ctx = NULL;

    /* Initialiser les données */
    memset(&g_donnees_recherche, 0, sizeof(g_donnees_recherche));

    /* Enregistrer la classe du dialogue */
    static BOOL classe_ok = FALSE;
    if (!classe_ok) {
        WNDCLASSEXW wc = {0};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = recherche_proc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"IntelliEditorRecherche";
        RegisterClassExW(&wc);
        classe_ok = TRUE;
    }

    /* Créer la fenêtre de dialogue */
    HWND hwnd_dlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"IntelliEditorRecherche",
        L"Rechercher / Remplacer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 200,
        parent, NULL,
        GetModuleHandle(NULL), NULL
    );

    if (!hwnd_dlg) return 0;

    ShowWindow(hwnd_dlg, SW_SHOW);
    UpdateWindow(hwnd_dlg);

    /* Boucle de messages modale */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!IsWindow(hwnd_dlg)) break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (!g_donnees_recherche.valide ||
        g_donnees_recherche.recherche[0] == '\0') {
        return 0;
    }

    /* Créer le contexte de recherche */
    *out_ctx = recherche_creer(
        g_donnees_recherche.recherche,
        g_donnees_recherche.utiliser_regex,
        g_donnees_recherche.sensible_casse
    );

    if (!*out_ctx) {
        dialog_erreur(parent,
            "Expression régulière invalide.\n"
            "Vérifiez la syntaxe PCRE2.");
        return 0;
    }

    /* Copier le remplacement */
    if (out_remplacement && taille_remplacement > 0) {
        strncpy(out_remplacement,
                g_donnees_recherche.remplacement,
                (size_t)(taille_remplacement - 1));
        out_remplacement[taille_remplacement - 1] = '\0';
    }

    return 1;
}

/* --------------------------------------------------------------------------
 * dialog_erreur
 * -------------------------------------------------------------------------- */
void dialog_erreur(HWND parent, const char *message)
{
    if (!message) return;
    wchar_t msg_w[512];
    MultiByteToWideChar(CP_UTF8, 0, message, -1, msg_w, 512);
    MessageBoxW(parent, msg_w, L"Erreur — IntelliEditor",
                MB_ICONERROR | MB_OK);
}

/* --------------------------------------------------------------------------
 * dialog_info
 * -------------------------------------------------------------------------- */
void dialog_info(HWND parent, const char *message)
{
    if (!message) return;
    wchar_t msg_w[512];
    MultiByteToWideChar(CP_UTF8, 0, message, -1, msg_w, 512);
    MessageBoxW(parent, msg_w, L"Information — IntelliEditor",
                MB_ICONINFORMATION | MB_OK);
}

/* --------------------------------------------------------------------------
 * dialog_confirmer
 * -------------------------------------------------------------------------- */
int dialog_confirmer(HWND parent, const char *question)
{
    if (!question) return 0;
    wchar_t msg_w[512];
    MultiByteToWideChar(CP_UTF8, 0, question, -1, msg_w, 512);
    int res = MessageBoxW(parent, msg_w, L"Confirmation — IntelliEditor",
                          MB_ICONQUESTION | MB_YESNO);
    return (res == IDYES) ? 1 : 0;
}