#define _WIN32_WINNT 0x0600  /* Windows Vista minimum — requis pour ICC_* */
#define WINVER       0x0600



#include "ui/main_window.h"
#include "ui/scintilla_wrapper.h"
#include "utils/config.h"
#include "utils/encoding.h"
#include "nlp/hunspell_wrap.h"
#include "nlp/nlp_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>   /* INITCOMMONCONTROLSEX, InitCommonControlsEx */

/* Constantes SCI_* utilisées directement dans main_window.c */
#ifndef SCI_CLEARALL
#define SCI_CLEARALL   2004
#endif
#ifndef SCI_UNDO
#define SCI_UNDO       2176
#endif
#ifndef SCI_REDO
#define SCI_REDO       2011
#endif
#ifndef SCI_CUT
#define SCI_CUT        2177
#endif
#ifndef SCI_COPY
#define SCI_COPY       2178
#endif
#ifndef SCI_PASTE
#define SCI_PASTE      2179
#endif
#ifndef SCI_SELECTALL
#define SCI_SELECTALL  2013
#endif

/* -----------------------------------------------------------------------
 * Variables globales de la fenêtre
 * ----------------------------------------------------------------------- */
HWND g_hwnd_principale = NULL;
HWND g_hwnd_scintilla  = NULL;

/* Largeur réservée pour le panneau de règles (droite) */
#define LARGEUR_PANNEAU_REGLES 250

/* -----------------------------------------------------------------------
 * Création du menu principal
 * ----------------------------------------------------------------------- */
static HMENU creer_menu(void) {
    HMENU menu_bar   = CreateMenu();
    HMENU m_fichier  = CreatePopupMenu();
    HMENU m_edition  = CreatePopupMenu();
    HMENU m_affichage = CreatePopupMenu();
    HMENU m_outils   = CreatePopupMenu();
    HMENU m_reformuler = CreatePopupMenu();
    HMENU m_aide     = CreatePopupMenu();

    /* --- Fichier --- */
    AppendMenuW(m_fichier, MF_STRING, ID_FICHIER_NOUVEAU,
                L"&Nouveau\tCtrl+N");
    AppendMenuW(m_fichier, MF_STRING, ID_FICHIER_OUVRIR,
                L"&Ouvrir...\tCtrl+O");
    AppendMenuW(m_fichier, MF_STRING, ID_FICHIER_ENREGISTRER,
                L"&Enregistrer\tCtrl+S");
    AppendMenuW(m_fichier, MF_STRING, ID_FICHIER_ENREG_SOUS,
                L"Enregistrer &sous...\tCtrl+Shift+S");
    AppendMenuW(m_fichier, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m_fichier, MF_STRING, ID_FICHIER_QUITTER,
                L"&Quitter\tAlt+F4");

    /* --- Édition --- */
    AppendMenuW(m_edition, MF_STRING, ID_EDITION_ANNULER,
                L"&Annuler\tCtrl+Z");
    AppendMenuW(m_edition, MF_STRING, ID_EDITION_REFAIRE,
                L"&Refaire\tCtrl+Shift+Z");
    AppendMenuW(m_edition, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m_edition, MF_STRING, ID_EDITION_COUPER,
                L"Co&uper\tCtrl+X");
    AppendMenuW(m_edition, MF_STRING, ID_EDITION_COPIER,
                L"&Copier\tCtrl+C");
    AppendMenuW(m_edition, MF_STRING, ID_EDITION_COLLER,
                L"C&oller\tCtrl+V");
    AppendMenuW(m_edition, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m_edition, MF_STRING, ID_EDITION_TOUT_SELEC,
                L"Tout &sélectionner\tCtrl+A");
    AppendMenuW(m_edition, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m_edition, MF_STRING, ID_EDITION_RECHERCHER,
                L"&Rechercher...\tCtrl+F");
    AppendMenuW(m_edition, MF_STRING, ID_EDITION_REMPLACER,
                L"Re&mplacer...\tCtrl+H");

    /* --- Affichage --- */
    AppendMenuW(m_affichage, MF_STRING, ID_AFFICHAGE_THEME,
                L"Basculer thème clair/sombre");

    /* --- Outils > Reformuler (sous-menu) --- */
    AppendMenuW(m_reformuler, MF_STRING, ID_OUTILS_REFORMULER_FORMEL,
                L"Style &formel");
    AppendMenuW(m_reformuler, MF_STRING, ID_OUTILS_REFORMULER_SIMPLE,
                L"Style &simplifié");
    AppendMenuW(m_reformuler, MF_STRING, ID_OUTILS_REFORMULER_ACADEM,
                L"Style &académique");

    AppendMenuW(m_outils, MF_STRING, ID_OUTILS_REGLES,
                L"Charger les rè&gles...");
    AppendMenuW(m_outils, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m_outils, MF_POPUP, (UINT_PTR)m_reformuler,
                L"Re&formuler");
    AppendMenuW(m_outils, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m_outils, MF_STRING, ID_OUTILS_PREFERENCES,
                L"&Préférences...");

    /* --- Aide --- */
    AppendMenuW(m_aide, MF_STRING, ID_AIDE_APROPOS,
                L"&À propos...");

    /* Assemblage de la barre de menus */
    AppendMenuW(menu_bar, MF_POPUP, (UINT_PTR)m_fichier,   L"&Fichier");
    AppendMenuW(menu_bar, MF_POPUP, (UINT_PTR)m_edition,   L"&Édition");
    AppendMenuW(menu_bar, MF_POPUP, (UINT_PTR)m_affichage, L"&Affichage");
    AppendMenuW(menu_bar, MF_POPUP, (UINT_PTR)m_outils,    L"&Outils");
    AppendMenuW(menu_bar, MF_POPUP, (UINT_PTR)m_aide,      L"&Aide");

    return menu_bar;
}

/* -----------------------------------------------------------------------
 * Création de la table d'accélérateurs clavier
 * ----------------------------------------------------------------------- */
static HACCEL creer_accelerateurs(void) {
    ACCEL acc[] = {
        { FCONTROL | FVIRTKEY, 'N', ID_FICHIER_NOUVEAU      },
        { FCONTROL | FVIRTKEY, 'O', ID_FICHIER_OUVRIR       },
        { FCONTROL | FVIRTKEY, 'S', ID_FICHIER_ENREGISTRER  },
        { FCONTROL | FSHIFT | FVIRTKEY, 'S', ID_FICHIER_ENREG_SOUS },
        { FCONTROL | FVIRTKEY, 'Z', ID_EDITION_ANNULER      },
        { FCONTROL | FSHIFT | FVIRTKEY, 'Z', ID_EDITION_REFAIRE },
        { FCONTROL | FVIRTKEY, 'X', ID_EDITION_COUPER       },
        { FCONTROL | FVIRTKEY, 'C', ID_EDITION_COPIER       },
        { FCONTROL | FVIRTKEY, 'V', ID_EDITION_COLLER       },
        { FCONTROL | FVIRTKEY, 'A', ID_EDITION_TOUT_SELEC   },
        { FCONTROL | FVIRTKEY, 'F', ID_EDITION_RECHERCHER   },
        { FCONTROL | FVIRTKEY, 'H', ID_EDITION_REMPLACER    },
    };

    return CreateAcceleratorTableW(acc, sizeof(acc) / sizeof(acc[0]));
}

/* -----------------------------------------------------------------------
 * Gestion des commandes de menu (WM_COMMAND)
 * ----------------------------------------------------------------------- */
static void traiter_commande(HWND hwnd, WORD id_commande) {
    switch (id_commande) {

        case ID_FICHIER_NOUVEAU:
            /* Effacer le contenu de Scintilla */
            SendMessage(g_hwnd_scintilla, SCI_CLEARALL, 0, 0);
            ui_set_titre(L"Sans titre");
            break;

        case ID_FICHIER_QUITTER:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case ID_EDITION_ANNULER:
            SendMessage(g_hwnd_scintilla, SCI_UNDO, 0, 0);
            break;

        case ID_EDITION_REFAIRE:
            SendMessage(g_hwnd_scintilla, SCI_REDO, 0, 0);
            break;

        case ID_EDITION_COUPER:
            SendMessage(g_hwnd_scintilla, SCI_CUT, 0, 0);
            break;

        case ID_EDITION_COPIER:
            SendMessage(g_hwnd_scintilla, SCI_COPY, 0, 0);
            break;

        case ID_EDITION_COLLER:
            SendMessage(g_hwnd_scintilla, SCI_PASTE, 0, 0);
            break;

        case ID_EDITION_TOUT_SELEC:
            SendMessage(g_hwnd_scintilla, SCI_SELECTALL, 0, 0);
            break;

        case ID_AIDE_APROPOS:
            MessageBoxW(hwnd,
                L"IntelliEditor v1.0\n"
                L"Éditeur de texte intelligent hors ligne\n"
                L"Développé en C11 — UDBL 2025-2026",
                L"À propos d'IntelliEditor",
                MB_ICONINFORMATION | MB_OK);
            break;

        /* Les autres commandes (Ouvrir, Enregistrer, Règles, etc.)
         * seront branchées dans les phases suivantes */
        default:
            break;
    }
}

/* -----------------------------------------------------------------------
 * Procédure de la fenêtre principale (WndProc)
 * ----------------------------------------------------------------------- */
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg,
                                  WPARAM wParam, LPARAM lParam) {
    switch (msg) {

        case WM_CREATE:
            /* Création du contrôle Scintilla */
            g_hwnd_scintilla = scintilla_creer(hwnd);
            if (!g_hwnd_scintilla) {
                MessageBoxW(hwnd,
                    L"Impossible de créer le contrôle Scintilla.",
                    L"Erreur critique",
                    MB_ICONERROR | MB_OK);
                return -1; /* Annule la création de la fenêtre */
            }

            /* Initialisation Hunspell + moteur NLP
             * On construit le chemin absolu depuis le répertoire de l'exe
             * pour que ça fonctionne quel que soit le répertoire courant. */
            {
                char chemin_aff[MAX_PATH];
                char chemin_dic[MAX_PATH];

                /* Récupère le chemin complet de l'exécutable */
                wchar_t chemin_exe_w[MAX_PATH];
                GetModuleFileNameW(NULL, chemin_exe_w, MAX_PATH);

                /* Convertit en UTF-8 */
                char chemin_exe[MAX_PATH];
                WideCharToMultiByte(CP_UTF8, 0, chemin_exe_w, -1,
                                    chemin_exe, MAX_PATH, NULL, NULL);

                /* Remonte au répertoire de l'exe (supprime le nom du .exe) */
                char *dernier_sep = strrchr(chemin_exe, '\\');
                if (!dernier_sep) dernier_sep = strrchr(chemin_exe, '/');
                if (dernier_sep) *(dernier_sep + 1) = '\0';

                /* Construit les chemins vers les dictionnaires.
                 * Les dictionnaires sont copiés à côté de l'exe au build,
                 * OU on pointe vers la racine du projet via le chemin exe. */
                snprintf(chemin_aff, MAX_PATH, "%sdata\\hunspell\\fr.aff",
                         chemin_exe);
                snprintf(chemin_dic, MAX_PATH, "%sdata\\hunspell\\fr.dic",
                         chemin_exe);

                /* Debug : affiche les chemins tentés dans stderr */
                fprintf(stderr, "[init] AFF : %s\n", chemin_aff);
                fprintf(stderr, "[init] DIC : %s\n", chemin_dic);

                if (!spell_init(chemin_aff, chemin_dic)) {
                    MessageBoxW(hwnd,
                        L"Dictionnaire français introuvable.\n"
                        L"La correction orthographique sera désactivée.\n\n"
                        L"Placez fr.aff et fr.dic dans :\n"
                        L"<dossier exe>\\data\\hunspell\\",
                        L"Avertissement",
                        MB_ICONWARNING | MB_OK);
                } else {
                    nlp_init();
                }
            }

            /* Timers */
            SetTimer(hwnd, TIMER_STATUT_ID, 300,  NULL);
            SetTimer(hwnd, TIMER_ORTHO_ID,  500,  NULL);
            SetTimer(hwnd, TIMER_REGLES_ID, 2000, NULL);
            return 0;

        case WM_SIZE: {
            /* Redimensionner Scintilla pour occuper tout l'espace
             * (moins le panneau de règles à droite) */
            int largeur_totale = LOWORD(lParam);
            int hauteur        = HIWORD(lParam);
            int largeur_sci    = largeur_totale - LARGEUR_PANNEAU_REGLES;
            if (largeur_sci < 100) largeur_sci = 100;

            scintilla_redimensionner(g_hwnd_scintilla,
                0, 0, largeur_sci, hauteur);
            return 0;
        }

        case WM_COMMAND:
            traiter_commande(hwnd, LOWORD(wParam));
            return 0;

        
        
        
        case WM_TIMER: {
            if (wParam == TIMER_ORTHO_ID) {
                /*
                 * Timer orthographe (500 ms) :
                 * Récupère le texte de Scintilla, lance la vérification
                 * Hunspell, efface les anciens soulignements et applique
                 * les nouveaux.
                 */
                char *texte = scintilla_get_texte(g_hwnd_scintilla);
                if (texte) {
                    int nb_erreurs = 0;
                    ErreurOrtho *erreurs = nlp_verifier_ortho(texte, &nb_erreurs);

                    /* Effacer tous les soulignements orthographiques */
                    scintilla_effacer_ortho(g_hwnd_scintilla);

                    /* Appliquer les nouveaux soulignements */
                    for (int i = 0; i < nb_erreurs; i++) {
                        scintilla_marquer_ortho(g_hwnd_scintilla,
                                                erreurs[i].debut,
                                                erreurs[i].longueur);
                    }

                    nlp_liberer_erreurs(erreurs, nb_erreurs);
                    free(texte);
                }
            }
            /* TIMER_REGLES_ID et TIMER_STATUT_ID seront branchés
             * dans les étapes suivantes de la Phase 2. */
            return 0;
        }     
             

        case WM_NOTIFY: {
            /* Notifications Scintilla (SCN_*) */
            NMHDR* nmhdr = (NMHDR*)lParam;
            if (nmhdr->hwndFrom == g_hwnd_scintilla) {
                /* Sera traité en Phase 2 */
            }
            return 0;
        }

        case WM_CLOSE:
            /* Confirmation avant fermeture (simplifiée pour l'instant) */
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_STATUT_ID);
            KillTimer(hwnd, TIMER_ORTHO_ID);
            KillTimer(hwnd, TIMER_REGLES_ID);
            nlp_shutdown();
            spell_shutdown();
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

/* -----------------------------------------------------------------------
 * Initialisation de la fenêtre principale
 * ----------------------------------------------------------------------- */
BOOL ui_init(HINSTANCE hInstance, int nCmdShow) {

    /* Initialiser les contrôles communs Windows (barre d'état, etc.) */
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    /* Charger Scintilla */
    if (!scintilla_charger()) return FALSE;

    /* Enregistrer la classe de fenêtre principale */
    WNDCLASSEXW wc    = {0};
    wc.cbSize         = sizeof(wc);
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = wnd_proc;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName  = CLASSE_FENETRE_PRINCIPALE;
    wc.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL,
            L"Impossible d'enregistrer la classe de fenêtre.",
            L"Erreur critique",
            MB_ICONERROR | MB_OK);
        return FALSE;
    }

    /* Créer le menu */
    HMENU menu = creer_menu();

    /* Créer la fenêtre principale */
    g_hwnd_principale = CreateWindowExW(
        0,
        CLASSE_FENETRE_PRINCIPALE,
        L"IntelliEditor — Sans titre",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 768,
        NULL,
        menu,
        hInstance,
        NULL
    );

    if (!g_hwnd_principale) {
        MessageBoxW(NULL,
            L"Impossible de créer la fenêtre principale.",
            L"Erreur critique",
            MB_ICONERROR | MB_OK);
        return FALSE;
    }

    ShowWindow(g_hwnd_principale, nCmdShow);
    UpdateWindow(g_hwnd_principale);

    return TRUE;
}

/* -----------------------------------------------------------------------
 * Boucle de messages
 * ----------------------------------------------------------------------- */
int ui_run(HINSTANCE hInstance) {
    HACCEL haccel = creer_accelerateurs();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!TranslateAcceleratorW(g_hwnd_principale, haccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (haccel) DestroyAcceleratorTable(haccel);
    return (int)msg.wParam;
}

/* -----------------------------------------------------------------------
 * Mise à jour du titre
 * ----------------------------------------------------------------------- */
void ui_set_titre(const wchar_t* nom_fichier) {
    if (!g_hwnd_principale) return;

    wchar_t titre[512];
    if (nom_fichier && *nom_fichier) {
        swprintf(titre, 512, L"IntelliEditor — %ls", nom_fichier);
    } else {
        swprintf(titre, 512, L"IntelliEditor — Sans titre");
    }
    SetWindowTextW(g_hwnd_principale, titre);
}