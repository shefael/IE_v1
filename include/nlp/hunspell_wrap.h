#ifndef NLP_HUNSPELL_WRAP_H
#define NLP_HUNSPELL_WRAP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialise Hunspell avec les fichiers .aff et .dic */
int spell_init(const char* aff_path, const char* dic_path);

/* Vérifie si un mot est correctement orthographié (retourne 1 si correct, 0 sinon) */
int spell_check(const char* word);

/* Propose des suggestions pour un mot mal orthographié (renvoie un tableau de chaînes) */
char** spell_suggest(const char* word, int* nsuggest);

/* Libère les ressources Hunspell */
void spell_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* NLP_HUNSPELL_WRAP_H */