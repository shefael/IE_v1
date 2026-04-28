#define _WIN32_WINNT 0x0600  /* Windows Vista minimum — requis pour ICC_* */
#define WINVER       0x0600



#include "ui/main_window.h"
#include "ui/scintilla_wrapper.h"
#include "utils/config.h"
#include "utils/encoding.h"
#include "nlp/hunspell_wrap.h"
#include "nlp/nlp_engine.h"
#include "ui/toolbar.h"
#include "ui/statusbar.h"
#include "ui/rules_panel.h"
#include "ui/dialogs.h"
#include "editor/formatter.h"
#include "editor/exporter.h"
#include "rules/rule_parser.h"
#include "rules/rule_engine.h"
#include "rules/rule_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>   /* INITCOMMONCONTROLSEX, InitCommonControlsEx */

/* Constantes SCI_* utilisées directement dans main_window.c */
#ifndef SCI_CLEARALL
#define SCI_CLEARALL   2004
#endif
#ifndef SCI_SETTEXT
#define SCI_SETTEXT    2181
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
HWND g_hwnd_principale  = NULL;
HWND g_hwnd_scintilla   = NULL;
HWND g_hwnd_toolbar     = NULL;
HWND g_hwnd_statusbar   = NULL;
HWND g_hwnd_rules_panel = NULL;
/* Table des styles globale (partagée entre formatter et exporter) */
static TableStyles *g_table_styles = NULL;

/* Ensemble de règles chargé */
static RuleSet *g_ruleset = NULL;

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
            if (dialog_confirmer(hwnd,
                    "Créer un nouveau document ?\n"
                    "Le contenu non enregistré sera perdu.")) {
                SendMessage(g_hwnd_scintilla, SCI_CLEARALL, 0, 0);
                if (g_table_styles) formatter_vider(g_table_styles);
                ui_set_titre(L"Sans titre");
            }
            break;

        case ID_FICHIER_OUVRIR: {
            char *chemin = dialog_ouvrir_fichier(hwnd);
            if (chemin) {
                char *contenu = importer_txt(chemin);
                if (contenu) {
                    /* Charger le texte dans Scintilla */
                    SendMessage(g_hwnd_scintilla, SCI_CLEARALL, 0, 0);
                    SendMessage(g_hwnd_scintilla, SCI_SETTEXT,  /* SCI_SETTEXT */
                                0, (LPARAM)contenu);
                    if (g_table_styles) formatter_vider(g_table_styles);
                    /* Mettre à jour le titre */
                    wchar_t chemin_w[MAX_PATH];
                    MultiByteToWideChar(CP_UTF8, 0, chemin, -1,
                                        chemin_w, MAX_PATH);
                    /* Extraire uniquement le nom du fichier */
                    wchar_t *nom = wcsrchr(chemin_w, L'\\');
                    ui_set_titre(nom ? nom + 1 : chemin_w);
                    free(contenu);
                } else {
                    dialog_erreur(hwnd,
                        "Impossible d'ouvrir le fichier.\n"
                        "Vérifiez que le fichier existe et est lisible.");
                }
                free(chemin);
            }
            break;
        }

        case ID_FICHIER_ENREGISTRER:
        case ID_FICHIER_ENREG_SOUS: {
            char *chemin = dialog_enregistrer_fichier(hwnd);
            if (chemin) {
                char *texte = scintilla_get_texte(g_hwnd_scintilla);
                if (texte) {
                    int ret = EXPORT_OK;
                    /* Déterminer le format selon l'extension.
                     * On cherche le dernier '.' dans le chemin. */
                    const char *ext = strrchr(chemin, '.');
                    if (ext && _stricmp(ext, ".rtf") == 0) {
                        ret = exporter_rtf(chemin, texte, g_table_styles);
                    } else if (ext && _stricmp(ext, ".ie") == 0) {
                        ret = exporter_ie(chemin, texte, g_table_styles);
                    } else {
                        ret = exporter_txt(chemin, texte);
                    }

                    if (ret != EXPORT_OK) {
                        dialog_erreur(hwnd,
                            "Erreur lors de l'enregistrement du fichier.\n"
                            "Vérifiez les droits d'accès et l'espace disque.");
                    } else {
                        wchar_t chemin_w[MAX_PATH];
                        MultiByteToWideChar(CP_UTF8, 0, chemin, -1,
                                            chemin_w, MAX_PATH);
                        wchar_t *nom = wcsrchr(chemin_w, L'\\');
                        ui_set_titre(nom ? nom + 1 : chemin_w);
                    }
                    free(texte);
                }
                free(chemin);
            }
            break;
        }

        case ID_OUTILS_REGLES: {
            char *chemin = dialog_ouvrir_regles(hwnd);
            if (chemin) {
                /* Libérer l'ancien ruleset */
                if (g_ruleset) {
                    ruleset_free(g_ruleset);
                    g_ruleset = NULL;
                }

                g_ruleset = rules_parse_file(chemin);
                if (!g_ruleset) {
                    dialog_erreur(hwnd,
                        "Impossible de charger le fichier de règles.\n"
                        "Vérifiez que le fichier JSON est valide.");
                } else {
                    /* Peupler le panneau de règles */
                    int nb = (int)g_ruleset->nb_regles;
                    ElementRegle *elements = (ElementRegle *)calloc((size_t)nb, sizeof(ElementRegle));
                    
                    if (elements) {
                        for (int i = 0; i < nb; i++) {
                            // CORRECTION : On ne teste plus l'adresse du tableau directement
                            // On copie simplement le contenu de manière sécurisée
                            strncpy(elements[i].id, 
                                    g_ruleset->regles[i].id, 
                                    sizeof(elements[i].id) - 1);
                            elements[i].id[sizeof(elements[i].id) - 1] = '\0';

                            strncpy(elements[i].description, 
                                    g_ruleset->regles[i].description, 
                                    sizeof(elements[i].description) - 1);
                            elements[i].description[sizeof(elements[i].description) - 1] = '\0';

                            // CORRECTION : Utilisation du nouveau nom de constante
                            elements[i].statut = RULE_STATUS_PENDING;
                        }
                        
                        rules_panel_peupler(g_hwnd_rules_panel, elements, nb);
                        free(elements);
                    }

                    char msg[512];
                    snprintf(msg, sizeof(msg),
                             "%d règle(s) chargée(s) depuis :\n%s",
                             nb, chemin);
                    dialog_info(hwnd, msg);
                }
                free(chemin);
            }
            break;
        }

        case ID_EDITION_RECHERCHER:
        case ID_EDITION_REMPLACER: {
            ContexteRecherche *ctx = NULL;
            char remplacement[512] = {0};

            if (dialog_rechercher_remplacer(hwnd, &ctx,
                                             remplacement,
                                             sizeof(remplacement))) {
                if (ctx) {
                    char *texte = scintilla_get_texte(g_hwnd_scintilla);
                    if (texte) {
                        int nb = 0;
                        char *nouveau = recherche_remplacer_tout(
                            ctx, texte, remplacement, &nb);
                        if (nouveau && nb > 0) {
                            SendMessage(g_hwnd_scintilla, SCI_SETTEXT,
                                        0, (LPARAM)nouveau);
                            char msg[128];
                            snprintf(msg, sizeof(msg),
                                     "%d remplacement(s) effectué(s).", nb);
                            dialog_info(hwnd, msg);
                            free(nouveau);
                        } else if (nb == 0) {
                            dialog_info(hwnd,
                                "Aucune occurrence trouvée.");
                            free(nouveau);
                        }
                        free(texte);
                    }
                    recherche_detruire(ctx);
                }
            }
            break;
        }

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
        case ID_TB_GRAS:
            /* Application du style gras sur la sélection Scintilla.
             * Pour l'instant on notifie via la barre de statut (Phase 2).
             * Le style est stocké dans g_table_styles pour l'export RTF. */
            if (g_table_styles) {
                /* Récupère la sélection courante dans Scintilla */
                LRESULT debut_sel = SendMessage(g_hwnd_scintilla,
                                                2143, 0, 0); /* SCI_GETSELECTIONSTART */
                LRESULT fin_sel   = SendMessage(g_hwnd_scintilla,
                                                2144, 0, 0); /* SCI_GETSELECTIONEND */
                if (fin_sel > debut_sel) {
                    formatter_appliquer(g_table_styles,
                                        (size_t)debut_sel,
                                        (size_t)(fin_sel - debut_sel),
                                        STYLE_GRAS);
                }
            }
            break;

        case ID_TB_ITALIQUE:
            if (g_table_styles) {
                LRESULT debut_sel = SendMessage(g_hwnd_scintilla,
                                                2143, 0, 0);
                LRESULT fin_sel   = SendMessage(g_hwnd_scintilla,
                                                2144, 0, 0);
                if (fin_sel > debut_sel) {
                    formatter_appliquer(g_table_styles,
                                        (size_t)debut_sel,
                                        (size_t)(fin_sel - debut_sel),
                                        STYLE_ITALIQUE);
                }
            }
            break;

        case ID_TB_SOULIGNE:
            if (g_table_styles) {
                LRESULT debut_sel = SendMessage(g_hwnd_scintilla,
                                                2143, 0, 0);
                LRESULT fin_sel   = SendMessage(g_hwnd_scintilla,
                                                2144, 0, 0);
                if (fin_sel > debut_sel) {
                    formatter_appliquer(g_table_styles,
                                        (size_t)debut_sel,
                                        (size_t)(fin_sel - debut_sel),
                                        STYLE_SOULIGNE);
                }
            }
            break;

        case ID_TB_TITRE1:
            if (g_table_styles) {
                LRESULT debut_sel = SendMessage(g_hwnd_scintilla,
                                                2143, 0, 0);
                LRESULT fin_sel   = SendMessage(g_hwnd_scintilla,
                                                2144, 0, 0);
                if (fin_sel > debut_sel) {
                    formatter_appliquer(g_table_styles,
                                        (size_t)debut_sel,
                                        (size_t)(fin_sel - debut_sel),
                                        STYLE_TITRE_1);
                }
            }
            break;

        case ID_TB_TITRE2:
            if (g_table_styles) {
                LRESULT debut_sel = SendMessage(g_hwnd_scintilla,
                                                2143, 0, 0);
                LRESULT fin_sel   = SendMessage(g_hwnd_scintilla,
                                                2144, 0, 0);
                if (fin_sel > debut_sel) {
                    formatter_appliquer(g_table_styles,
                                        (size_t)debut_sel,
                                        (size_t)(fin_sel - debut_sel),
                                        STYLE_TITRE_2);
                }
            }
            break;

        case ID_TB_TITRE3:
            if (g_table_styles) {
                LRESULT debut_sel = SendMessage(g_hwnd_scintilla,
                                                2143, 0, 0);
                LRESULT fin_sel   = SendMessage(g_hwnd_scintilla,
                                                2144, 0, 0);
                if (fin_sel > debut_sel) {
                    formatter_appliquer(g_table_styles,
                                        (size_t)debut_sel,
                                        (size_t)(fin_sel - debut_sel),
                                        STYLE_TITRE_3);
                }
            }
            break;

        /* Les autres commandes seront branchées dans les étapes suivantes */
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
            /* Création de la table des styles */
            g_table_styles = formatter_creer();

            /* Création de la barre d'outils */
            g_hwnd_toolbar = toolbar_creer(hwnd);

            /* Création de la barre d'état */
            g_hwnd_statusbar = statusbar_creer(hwnd);

            /* Création du panneau de règles (position provisoire,
             * sera ajustée par le premier WM_SIZE) */
            g_hwnd_rules_panel = rules_panel_creer(hwnd,
                0, 0, LARGEUR_PANNEAU_REGLES, 400);

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
            int largeur_totale = LOWORD(lParam);
            int hauteur_totale = HIWORD(lParam);

            /* Toolbar : se redimensionne seule en largeur */
            int hauteur_tb = 0;
            if (g_hwnd_toolbar) {
                SendMessage(g_hwnd_toolbar, WM_SIZE, wParam, lParam);
                hauteur_tb = toolbar_get_hauteur(g_hwnd_toolbar);
            }

            /* Statusbar : se redimensionne seule en largeur */
            int hauteur_sb = 0;
            if (g_hwnd_statusbar) {
                SendMessage(g_hwnd_statusbar, WM_SIZE, wParam, lParam);
                hauteur_sb = statusbar_get_hauteur(g_hwnd_statusbar);
            }

            /* Scintilla occupe l'espace entre toolbar et statusbar */
            int largeur_sci = largeur_totale - LARGEUR_PANNEAU_REGLES;
            if (largeur_sci < 100) largeur_sci = 100;
            int hauteur_sci = hauteur_totale - hauteur_tb - hauteur_sb;
            if (hauteur_sci < 0) hauteur_sci = 0;

            scintilla_redimensionner(g_hwnd_scintilla,
                0, hauteur_tb, largeur_sci, hauteur_sci);

            /* Panneau de règles : à droite de Scintilla */
            if (g_hwnd_rules_panel) {
                rules_panel_redimensionner(g_hwnd_rules_panel,
                    largeur_sci, hauteur_tb,
                    LARGEUR_PANNEAU_REGLES, hauteur_sci);
            }
            return 0;
        }

        case WM_COMMAND:
            traiter_commande(hwnd, LOWORD(wParam));
            return 0;

        
        
        
        case WM_TIMER: {
            if (wParam == TIMER_STATUT_ID) {
                /*
                 * Timer statut (300 ms) :
                 * Met à jour le compteur de mots et la position curseur.
                 */
                if (g_hwnd_statusbar && g_hwnd_scintilla) {
                    size_t nb_mots  = scintilla_compter_mots(g_hwnd_scintilla);
                    size_t nb_chars = scintilla_get_longueur(g_hwnd_scintilla);
                    int    ligne    = scintilla_get_ligne_courante(g_hwnd_scintilla);
                    int    colonne  = scintilla_get_colonne_courante(g_hwnd_scintilla);

                    statusbar_maj_mots(g_hwnd_statusbar, nb_mots, nb_chars);
                    statusbar_maj_position(g_hwnd_statusbar, ligne, colonne);
                }
            } else if (wParam == TIMER_ORTHO_ID) {
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
            
            } else if (wParam == TIMER_REGLES_ID) {
                /*
                 * Timer règles (2000 ms) :
                 * Génère le rapport de conformité si un ruleset est chargé.
                 * Le debounce est naturellement assuré par le timer.
                 */
                if (g_ruleset && g_hwnd_rules_panel && g_hwnd_scintilla) {
                    char *texte = scintilla_get_texte(g_hwnd_scintilla);
                    if (texte) {
                        RuleReport *rapport = rule_report_generer(
                            g_ruleset, texte);
                        free(texte);

                        if (rapport) {
                            /* Mettre à jour le panneau règle par règle */
                            for (size_t i = 0; i < rapport->nb; i++) {
                                rules_panel_maj_statut(
                                    g_hwnd_rules_panel,
                                    rapport->items[i].rule_id,
                                    rapport->items[i].statut,
                                    rapport->items[i].message);
                            }

                            /* Mettre à jour la barre d'état */
                            if (g_hwnd_statusbar) {
                                char msg_llm[64];
                                snprintf(msg_llm, sizeof(msg_llm),
                                         "Règles : %d OK  %d warn  %d viol",
                                         rapport->nb_ok,
                                         rapport->nb_warn,
                                         rapport->nb_viol);
                                statusbar_maj_llm(g_hwnd_statusbar, msg_llm);
                            }

                            rule_report_free(rapport);
                        }
                    }
                }
            }
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
            if (g_table_styles) {
                formatter_detruire(g_table_styles);
                g_table_styles = NULL;
            }
            if (g_ruleset) {
                ruleset_free(g_ruleset);
                g_ruleset = NULL;
            }
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