/*
 * statusbar.c — Barre d'état multi-panneaux
 *
 * Utilise la classe msctls_statusbar32 de commctrl.
 * Les 4 panneaux sont définis par leurs largeurs en pixels.
 * Le dernier panneau (-1) s'étend jusqu'au bord droit.
 */

#include "ui/statusbar.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

/* Indices des panneaux */
#define PANNEAU_MOTS      0
#define PANNEAU_POSITION  1
#define PANNEAU_ENCODAGE  2
#define PANNEAU_LLM       3
#define NB_PANNEAUX       4

/* --------------------------------------------------------------------------
 * statusbar_creer
 * -------------------------------------------------------------------------- */
HWND statusbar_creer(HWND parent)
{
    if (!parent) return NULL;

    HWND hwnd_sb = CreateWindowExW(
        0,
        STATUSCLASSNAMEW,
        NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        parent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!hwnd_sb) return NULL;

    /*
     * Définir les largeurs des panneaux.
     * Chaque valeur est la coordonnée X du bord DROIT du panneau.
     * -1 pour le dernier = s'étend jusqu'au bord droit de la fenêtre.
     */
    int largeurs[NB_PANNEAUX] = { 220, 380, 480, -1 };
    SendMessage(hwnd_sb, SB_SETPARTS, NB_PANNEAUX, (LPARAM)largeurs);

    /* Valeurs initiales */
    SendMessageW(hwnd_sb, SB_SETTEXTW, PANNEAU_MOTS,
                 (LPARAM)L" Mots : 0  |  Caractères : 0");
    SendMessageW(hwnd_sb, SB_SETTEXTW, PANNEAU_POSITION,
                 (LPARAM)L" Ligne 1, Col 1");
    SendMessageW(hwnd_sb, SB_SETTEXTW, PANNEAU_ENCODAGE,
                 (LPARAM)L" UTF-8");
    SendMessageW(hwnd_sb, SB_SETTEXTW, PANNEAU_LLM,
                 (LPARAM)L" LLM : chargement...");

    return hwnd_sb;
}

/* --------------------------------------------------------------------------
 * statusbar_maj_mots
 * -------------------------------------------------------------------------- */
void statusbar_maj_mots(HWND hwnd_sb, size_t nb_mots, size_t nb_chars)
{
    if (!hwnd_sb) return;

    wchar_t buf[128];
    swprintf(buf, 128, L" Mots : %zu  |  Caractères : %zu",
             nb_mots, nb_chars);
    SendMessageW(hwnd_sb, SB_SETTEXTW, PANNEAU_MOTS, (LPARAM)buf);
}

/* --------------------------------------------------------------------------
 * statusbar_maj_position
 * -------------------------------------------------------------------------- */
void statusbar_maj_position(HWND hwnd_sb, int ligne, int colonne)
{
    if (!hwnd_sb) return;

    wchar_t buf[64];
    swprintf(buf, 64, L" Ligne %d, Col %d", ligne, colonne);
    SendMessageW(hwnd_sb, SB_SETTEXTW, PANNEAU_POSITION, (LPARAM)buf);
}

/* --------------------------------------------------------------------------
 * statusbar_maj_llm
 * -------------------------------------------------------------------------- */
void statusbar_maj_llm(HWND hwnd_sb, const char *statut)
{
    if (!hwnd_sb || !statut) return;

    wchar_t buf[128];
    wchar_t statut_w[96];
    MultiByteToWideChar(CP_UTF8, 0, statut, -1, statut_w, 96);
    swprintf(buf, 128, L" LLM : %ls", statut_w);
    SendMessageW(hwnd_sb, SB_SETTEXTW, PANNEAU_LLM, (LPARAM)buf);
}

/* --------------------------------------------------------------------------
 * statusbar_get_hauteur
 * -------------------------------------------------------------------------- */
int statusbar_get_hauteur(HWND hwnd_sb)
{
    if (!hwnd_sb) return 22;

    RECT rect;
    if (GetWindowRect(hwnd_sb, &rect)) {
        return rect.bottom - rect.top;
    }
    return 22;
}