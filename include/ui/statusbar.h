#ifndef UI_STATUSBAR_H
#define UI_STATUSBAR_H

/*
 * statusbar.h — Barre d'état multi-panneaux
 *
 * 4 panneaux :
 *   [0] Mots et caractères
 *   [1] Ligne et colonne
 *   [2] Encodage
 *   [3] Statut LLM
 */

#include <windows.h>
#include <stddef.h>

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Crée la barre d'état enfant de `parent`.
 * Retourne le HWND, NULL en cas d'échec.
 */
HWND statusbar_creer(HWND parent);

/*
 * Met à jour le panneau des mots et caractères.
 */
void statusbar_maj_mots(HWND hwnd_sb, size_t nb_mots, size_t nb_chars);

/*
 * Met à jour le panneau ligne/colonne.
 */
void statusbar_maj_position(HWND hwnd_sb, int ligne, int colonne);

/*
 * Met à jour le panneau statut LLM.
 */
void statusbar_maj_llm(HWND hwnd_sb, const char *statut);

/*
 * Retourne la hauteur de la barre d'état.
 */
int statusbar_get_hauteur(HWND hwnd_sb);

#endif /* UI_STATUSBAR_H */