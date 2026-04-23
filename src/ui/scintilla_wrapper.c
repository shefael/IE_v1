#include "ui/scintilla_wrapper.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

/* On inclut uniquement Scintilla.h (compatible C) */
#include "Scintilla.h"

/*
 * ScintillaMessages.h est un header C++ uniquement.
 * On définit ici les constantes SCI_* dont on a besoin
 * en copiant leurs valeurs depuis la doc Scintilla.
 */
#ifndef SCI_SETCODEPAGE
#define SCI_SETCODEPAGE         2037
#endif
#ifndef SCI_SETWRAPMODE
#define SCI_SETWRAPMODE         2268
#endif
#ifndef SCI_SETMARGINTYPEN
#define SCI_SETMARGINTYPEN      2240
#endif
#ifndef SCI_SETMARGINWIDTHN
#define SCI_SETMARGINWIDTHN     2242
#endif
#ifndef SCI_STYLESETFONT
#define SCI_STYLESETFONT        2056
#endif
#ifndef SCI_STYLESETSIZE
#define SCI_STYLESETSIZE        2055
#endif
#ifndef SCI_STYLECLEARALL
#define SCI_STYLECLEARALL       2050
#endif
#ifndef SCI_STYLESETBACK
#define SCI_STYLESETBACK        2052
#endif
#ifndef SCI_STYLESETFORE
#define SCI_STYLESETFORE        2051
#endif
#ifndef SCI_SETSELBACK
#define SCI_SETSELBACK          2068
#endif
#ifndef SCI_SETCARETFORE
#define SCI_SETCARETFORE        2069
#endif
#ifndef SCI_SETCARETWIDTH
#define SCI_SETCARETWIDTH       2188
#endif
#ifndef SCI_INDICSETSTYLE
#define SCI_INDICSETSTYLE       2080
#endif
#ifndef SCI_INDICSETFORE
#define SCI_INDICSETFORE        2082
#endif
#ifndef SCI_INDICSETALPHA
#define SCI_INDICSETALPHA       2523
#endif
#ifndef SCI_SETINDICATORCURRENT
#define SCI_SETINDICATORCURRENT 2500
#endif
#ifndef SCI_INDICATORFILLRANGE
#define SCI_INDICATORFILLRANGE  2504
#endif
#ifndef SCI_INDICATORCLEARRANGE
#define SCI_INDICATORCLEARRANGE 2502
#endif
#ifndef SCI_GETLENGTH
#define SCI_GETLENGTH           2006
#endif
#ifndef SCI_GETTEXT
#define SCI_GETTEXT             2182
#endif
#ifndef SCI_GETCURRENTPOS
#define SCI_GETCURRENTPOS       2008
#endif
#ifndef SCI_LINEFROMPOSITION
#define SCI_LINEFROMPOSITION    2166
#endif
#ifndef SCI_GETCOLUMN
#define SCI_GETCOLUMN           2129
#endif
#ifndef SCI_CLEARALL
#define SCI_CLEARALL            2004
#endif

/* Constantes Scintilla */
#ifndef SC_CP_UTF8
#define SC_CP_UTF8       65001
#endif
#ifndef SC_WRAP_WORD
#define SC_WRAP_WORD     1
#endif
#ifndef SC_MARGIN_NUMBER
#define SC_MARGIN_NUMBER 1
#endif
#ifndef SC_MARGIN_SYMBOL
#define SC_MARGIN_SYMBOL 0
#endif
#ifndef STYLE_DEFAULT
#define STYLE_DEFAULT    32
#endif
#ifndef STYLE_LINENUMBER
#define STYLE_LINENUMBER 33
#endif
#ifndef INDIC_SQUIGGLE
#define INDIC_SQUIGGLE   1
#endif

/* Macro d'envoi de message Scintilla */
#define SCI_MSG(hwnd, msg, wp, lp) \
    SendMessage((hwnd), (msg), (WPARAM)(wp), (LPARAM)(lp))

/* -----------------------------------------------------------------------
 * Chargement de Scintilla (compilé en statique)
 * ----------------------------------------------------------------------- */
BOOL scintilla_charger(void) {
    extern int Scintilla_RegisterClasses(void* hInstance);
    HINSTANCE hInst = GetModuleHandle(NULL);
    if (!Scintilla_RegisterClasses(hInst)) {
        MessageBoxW(NULL,
            L"Impossible d'initialiser Scintilla.",
            L"Erreur IntelliEditor",
            MB_ICONERROR | MB_OK);
        return FALSE;
    }
    return TRUE;
}

/* -----------------------------------------------------------------------
 * Création du contrôle
 * ----------------------------------------------------------------------- */
HWND scintilla_creer(HWND parent) {
    HWND hwnd = CreateWindowExW(
        0,
        L"Scintilla",
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 100, 100,
        parent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    if (!hwnd) return NULL;

    SCI_MSG(hwnd, SCI_SETCODEPAGE,      SC_CP_UTF8,    0);
    SCI_MSG(hwnd, SCI_SETWRAPMODE,      SC_WRAP_WORD,  0);

    /* Marge 0 : numéros de lignes */
    SCI_MSG(hwnd, SCI_SETMARGINTYPEN,   0, SC_MARGIN_NUMBER);
    SCI_MSG(hwnd, SCI_SETMARGINWIDTHN,  0, 40);

    /* Marge 1 : symboles */
    SCI_MSG(hwnd, SCI_SETMARGINTYPEN,   1, SC_MARGIN_SYMBOL);
    SCI_MSG(hwnd, SCI_SETMARGINWIDTHN,  1, 0);

    /* Police */
    SCI_MSG(hwnd, SCI_STYLESETFONT,     STYLE_DEFAULT, (LPARAM)"Consolas");
    SCI_MSG(hwnd, SCI_STYLESETSIZE,     STYLE_DEFAULT, 11);
    SCI_MSG(hwnd, SCI_STYLECLEARALL,    0, 0);

    /* Couleurs thème clair */
    SCI_MSG(hwnd, SCI_STYLESETBACK,     STYLE_DEFAULT,    RGB(255, 255, 255));
    SCI_MSG(hwnd, SCI_STYLESETFORE,     STYLE_DEFAULT,    RGB(30,  30,  30));
    SCI_MSG(hwnd, SCI_STYLESETBACK,     STYLE_LINENUMBER, RGB(240, 240, 240));
    SCI_MSG(hwnd, SCI_STYLESETFORE,     STYLE_LINENUMBER, RGB(120, 120, 120));
    SCI_MSG(hwnd, SCI_SETSELBACK,       TRUE,             RGB(173, 214, 255));
    SCI_MSG(hwnd, SCI_SETCARETFORE,     RGB(0, 0, 0),     0);
    SCI_MSG(hwnd, SCI_SETCARETWIDTH,    2,                0);

    scintilla_configurer_indicateurs(hwnd);
    return hwnd;
}

void scintilla_redimensionner(HWND hwnd_sci, int x, int y,
                               int largeur, int hauteur) {
    if (!hwnd_sci) return;
    SetWindowPos(hwnd_sci, NULL, x, y, largeur, hauteur,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void scintilla_configurer_indicateurs(HWND hwnd_sci) {
    if (!hwnd_sci) return;
    /* Indicateur 0 : orthographe (rouge ondulé) */
    SCI_MSG(hwnd_sci, SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
    SCI_MSG(hwnd_sci, SCI_INDICSETFORE,  0, RGB(255, 0, 0));
    SCI_MSG(hwnd_sci, SCI_INDICSETALPHA, 0, 255);
    /* Indicateur 1 : grammaire (bleu ondulé) */
    SCI_MSG(hwnd_sci, SCI_INDICSETSTYLE, 1, INDIC_SQUIGGLE);
    SCI_MSG(hwnd_sci, SCI_INDICSETFORE,  1, RGB(0, 100, 255));
    SCI_MSG(hwnd_sci, SCI_INDICSETALPHA, 1, 255);
}

void scintilla_marquer_ortho(HWND hwnd_sci, size_t start, size_t len) {
    if (!hwnd_sci || len == 0) return;
    SCI_MSG(hwnd_sci, SCI_SETINDICATORCURRENT, 0, 0);
    SCI_MSG(hwnd_sci, SCI_INDICATORFILLRANGE,  start, len);
}

void scintilla_effacer_ortho(HWND hwnd_sci) {
    if (!hwnd_sci) return;
    size_t lon = scintilla_get_longueur(hwnd_sci);
    if (lon == 0) return;
    SCI_MSG(hwnd_sci, SCI_SETINDICATORCURRENT, 0, 0);
    SCI_MSG(hwnd_sci, SCI_INDICATORCLEARRANGE, 0, lon);
}

char* scintilla_get_texte(HWND hwnd_sci) {
    if (!hwnd_sci) return NULL;
    size_t lon = (size_t)SCI_MSG(hwnd_sci, SCI_GETLENGTH, 0, 0);
    char* buf = (char*)malloc(lon + 1);
    if (!buf) return NULL;
    SCI_MSG(hwnd_sci, SCI_GETTEXT, lon + 1, buf);
    buf[lon] = '\0';
    return buf;
}

size_t scintilla_get_longueur(HWND hwnd_sci) {
    if (!hwnd_sci) return 0;
    return (size_t)SCI_MSG(hwnd_sci, SCI_GETLENGTH, 0, 0);
}

int scintilla_get_ligne_courante(HWND hwnd_sci) {
    if (!hwnd_sci) return 1;
    LRESULT pos   = SCI_MSG(hwnd_sci, SCI_GETCURRENTPOS,    0,   0);
    LRESULT ligne = SCI_MSG(hwnd_sci, SCI_LINEFROMPOSITION, pos, 0);
    return (int)(ligne + 1);
}

int scintilla_get_colonne_courante(HWND hwnd_sci) {
    if (!hwnd_sci) return 1;
    LRESULT pos = SCI_MSG(hwnd_sci, SCI_GETCURRENTPOS, 0,   0);
    LRESULT col = SCI_MSG(hwnd_sci, SCI_GETCOLUMN,     pos, 0);
    return (int)(col + 1);
}

size_t scintilla_compter_mots(HWND hwnd_sci) {
    if (!hwnd_sci) return 0;
    char* texte = scintilla_get_texte(hwnd_sci);
    if (!texte) return 0;
    size_t compte = 0;
    int dans_mot = 0;
    const unsigned char* p = (const unsigned char*)texte;
    while (*p) {
        int est_mot = isalnum(*p) || *p >= 0x80;
        if (est_mot && !dans_mot) { compte++; dans_mot = 1; }
        else if (!est_mot)         { dans_mot = 0; }
        p++;
    }
    free(texte);
    return compte;
}