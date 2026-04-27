#ifndef UI_RULES_PANEL_H
#define UI_RULES_PANEL_H

/*
 * rules_panel.h — Panneau latéral de conformité des règles
 *
 * Affiche la liste des règles chargées avec leur statut :
 *   ✅ Conforme     (STATUS_OK)
 *   ⚠  Avertissement (STATUS_WARNING)
 *   ❌ Non conforme  (STATUS_VIOLATION)
 *   .. En cours      (RULE_STATUS_PENDING)
 *
 * Implémenté avec un contrôle SysListView32 en mode Report.
 */

#include <windows.h>
#include "rules/rule_engine.h"

/* --------------------------------------------------------------------------
 * Structure représentant un élément affiché dans le panneau
 * -------------------------------------------------------------------------- */
typedef struct {
    char       id[32];          /* ID de la règle ex: "R001" */
    RuleStatus statut;          /* Statut courant */
    char       description[256]; /* Description courte */
    char       message[256];    /* Message de résultat (optionnel) */
} ElementRegle;

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Crée le panneau de règles enfant de `parent`.
 * x, y, largeur, hauteur : position et dimensions initiales.
 * Retourne le HWND du panneau, NULL en cas d'échec.
 */
HWND rules_panel_creer(HWND parent, int x, int y,
                        int largeur, int hauteur);

/*
 * Redimensionne le panneau.
 */
void rules_panel_redimensionner(HWND hwnd_panel, int x, int y,
                                 int largeur, int hauteur);

/*
 * Remplit le panneau avec un tableau d'éléments.
 * Efface la liste existante avant de remplir.
 */
void rules_panel_peupler(HWND hwnd_panel,
                          const ElementRegle *elements,
                          int nb_elements);

/*
 * Met à jour le statut d'une règle identifiée par son ID.
 * Ne repeuples pas toute la liste (mise à jour unitaire).
 */
void rules_panel_maj_statut(HWND hwnd_panel, const char *rule_id,
                             RuleStatus statut, const char *message);

/*
 * Vide le panneau et affiche un message informatif.
 */
void rules_panel_vider(HWND hwnd_panel);

/*
 * Convertit un RuleStatus en chaîne d'affichage avec icône.
 */
const wchar_t *rules_panel_statut_texte(RuleStatus statut);

#endif /* UI_RULES_PANEL_H */