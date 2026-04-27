#ifndef UI_DIALOGS_H
#define UI_DIALOGS_H

/*
 * dialogs.h — Boîtes de dialogue natives Windows
 *
 * Toutes les fonctions retournent des chaînes UTF-8 allouées dynamiquement
 * (libérer avec free()) ou NULL en cas d'annulation/erreur.
 */

#include <windows.h>
#include "editor/search_replace.h"

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Dialogue d'ouverture de fichier.
 * Filtres : TXT, RTF, IE, tous fichiers.
 * Retourne le chemin UTF-8 choisi, NULL si annulé.
 */
char *dialog_ouvrir_fichier(HWND parent);

/*
 * Dialogue d'enregistrement de fichier.
 * Filtres : TXT, RTF, IE.
 * Retourne le chemin UTF-8 choisi, NULL si annulé.
 */
char *dialog_enregistrer_fichier(HWND parent);

/*
 * Dialogue d'ouverture d'un fichier de règles JSON.
 * Retourne le chemin UTF-8 choisi, NULL si annulé.
 */
char *dialog_ouvrir_regles(HWND parent);

/*
 * Dialogue Rechercher/Remplacer.
 * Remplit *out_ctx avec un contexte de recherche créé.
 * Remplit out_remplacement (buffer fourni par l'appelant, taille MAX_PATH).
 * Retourne 1 si l'utilisateur a validé, 0 si annulé.
 */
int dialog_rechercher_remplacer(HWND parent,
                                 ContexteRecherche **out_ctx,
                                 char *out_remplacement,
                                 int taille_remplacement);

/*
 * Affiche un message d'erreur standardisé en français.
 */
void dialog_erreur(HWND parent, const char *message);

/*
 * Affiche un message d'information.
 */
void dialog_info(HWND parent, const char *message);

/*
 * Demande confirmation (Oui/Non).
 * Retourne 1 si Oui, 0 si Non.
 */
int dialog_confirmer(HWND parent, const char *question);

#endif /* UI_DIALOGS_H */