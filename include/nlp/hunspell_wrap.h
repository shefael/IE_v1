#ifndef NLP_HUNSPELL_WRAP_H
#define NLP_HUNSPELL_WRAP_H

/*
 * hunspell_wrap.h — Interface publique du wrapper Hunspell
 * Fournit la correction orthographique française via libhunspell.
 * Tous les textes en entrée/sortie sont en UTF-8.
 */

/*
 * Initialise Hunspell avec les fichiers de dictionnaire.
 * Retourne 1 en cas de succès, 0 en cas d'échec.
 * Chemins en UTF-8 (convertis en interne si nécessaire).
 */
int spell_init(const char *chemin_aff, const char *chemin_dic);

/*
 * Vérifie si un mot est correctement orthographié.
 * Retourne 1 si correct, 0 si incorrect.
 * Le mot doit être en UTF-8.
 */
int spell_verifier(const char *mot);

/*
 * Génère des suggestions pour un mot incorrect.
 * Retourne un tableau de chaînes allouées dynamiquement.
 * *nb_suggestions est mis à jour avec le nombre de suggestions.
 * Retourne NULL si aucune suggestion ou en cas d'erreur.
 * Libérer le résultat avec spell_liberer_suggestions().
 */
char **spell_suggerer(const char *mot, int *nb_suggestions);

/*
 * Libère les suggestions allouées par spell_suggerer().
 */
void spell_liberer_suggestions(char **suggestions, int nb_suggestions);

/*
 * Libère les ressources Hunspell.
 * À appeler avant la fermeture de l'application.
 */
void spell_shutdown(void);

/*
 * Ajoute un mot au dictionnaire de session (non persistant).
 * Utile pour les noms propres validés par l'utilisateur.
 */
void spell_ajouter_mot(const char *mot);

#endif /* NLP_HUNSPELL_WRAP_H */