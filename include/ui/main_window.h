#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialise l'interface utilisateur (fenêtre principale, Scintilla, etc.) */
int ui_init(void);

/* Lance la boucle de messages Windows */
int ui_run(void);

/* Nettoie les ressources de l'interface utilisateur */
void ui_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_MAIN_WINDOW_H */