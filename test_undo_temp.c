/* Fichier temporaire — NE PAS COMMITER — SUPPRIMER APRÈS VALIDATION */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "editor/gap_buffer.h"
#include "editor/undo_redo.h"

static void afficher(GapBuffer* gb, const char* label) {
    char* t = gb_get_text(gb);
    printf("  %-30s → '%s'\n", label, t ? t : "(null)");
    free(t);
}

int main(void) {
    printf("=== Test Undo/Redo ===\n");

    GapBuffer* gb = gb_create(64);
    History*   h  = history_create();

    /* Insert A */
    history_execute_insert(h, gb, 0, "Bonjour", 7);
    afficher(gb, "insert 'Bonjour'");

    /* Insert B */
    history_execute_insert(h, gb, 7, " monde", 6);
    afficher(gb, "insert ' monde'");

    /* Undo 1 : retire ' monde' */
    history_undo(h, gb);
    afficher(gb, "undo →");
    char* t = gb_get_text(gb);
    if (strcmp(t, "Bonjour") != 0) { printf("ECHEC undo 1\n"); free(t); return 1; }
    free(t);
    printf("  OK : undo 1\n");

    /* Redo : remet ' monde' */
    history_redo(h, gb);
    afficher(gb, "redo →");
    t = gb_get_text(gb);
    if (strcmp(t, "Bonjour monde") != 0) { printf("ECHEC redo\n"); free(t); return 1; }
    free(t);
    printf("  OK : redo\n");

    /* Insert C puis undo x2 */
    history_execute_insert(h, gb, 13, " !", 2);
    afficher(gb, "insert ' !'");

    history_undo(h, gb); /* retire ' !' */
    history_undo(h, gb); /* retire ' monde' */
    afficher(gb, "undo x2 →");
    t = gb_get_text(gb);
    if (strcmp(t, "Bonjour") != 0) { printf("ECHEC undo x2\n"); free(t); return 1; }
    free(t);
    printf("  OK : undo x2\n");

    /* Test replace atomique */
    history_execute_replace(h, gb, 0, "Bonjour", 7, "Salut", 5);
    afficher(gb, "replace 'Bonjour'→'Salut'");
    t = gb_get_text(gb);
    if (strcmp(t, "Salut") != 0) { printf("ECHEC replace\n"); free(t); return 1; }
    free(t);

    history_undo(h, gb); /* Un seul undo restaure 'Bonjour' */
    afficher(gb, "undo replace →");
    t = gb_get_text(gb);
    if (strcmp(t, "Bonjour") != 0) { printf("ECHEC undo replace\n"); free(t); return 1; }
    free(t);
    printf("  OK : replace atomique + undo\n");

    history_destroy(h);
    gb_destroy(gb);
    printf("=== Tous les tests passent ===\n");
    return 0;
}