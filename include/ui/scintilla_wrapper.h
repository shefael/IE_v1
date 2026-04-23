#ifndef UI_SCINTILLA_WRAPPER_H
#define UI_SCINTILLA_WRAPPER_H

#include <windows.h>
#include <stddef.h>

/*
 * Charge la DLL Scintilla et enregistre la classe "Scintilla".
 * Doit être appelé AVANT scintilla_creer().
 * Retourne TRUE en cas de succès.
 */
BOOL scintilla_charger(void);

/*
 * Crée le contrôle Scintilla en tant que fenêtre fille de 'parent'.
 * Retourne le HWND du contrôle, ou NULL en cas d'échec.
 */
HWND scintilla_creer(HWND parent);

/*
 * Redimensionne le contrôle Scintilla pour occuper la zone disponible.
 * A appeler dans le handler WM_SIZE de la fenêtre principale.
 */
void scintilla_redimensionner(HWND hwnd_sci, int x, int y, int largeur, int hauteur);

/*
 * Configure les indicateurs visuels (soulignements orthographe/grammaire).
 */
void scintilla_configurer_indicateurs(HWND hwnd_sci);

/*
 * Applique un soulignement rouge (orthographe) sur la plage [start, start+len[.
 */
void scintilla_marquer_ortho(HWND hwnd_sci, size_t start, size_t len);

/*
 * Efface tous les soulignements orthographiques.
 */
void scintilla_effacer_ortho(HWND hwnd_sci);

/*
 * Retourne le texte complet du contrôle Scintilla.
 * L'appelant doit libérer avec free().
 */
char* scintilla_get_texte(HWND hwnd_sci);

/*
 * Retourne la longueur du texte en octets.
 */
size_t scintilla_get_longueur(HWND hwnd_sci);

/*
 * Retourne le numéro de ligne courant (base 1).
 */
int scintilla_get_ligne_courante(HWND hwnd_sci);

/*
 * Retourne la colonne courante (base 1).
 */
int scintilla_get_colonne_courante(HWND hwnd_sci);

/*
 * Retourne le nombre de mots dans le texte courant.
 */
size_t scintilla_compter_mots(HWND hwnd_sci);

#endif /* UI_SCINTILLA_WRAPPER_H */