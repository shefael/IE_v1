#include "editor/gap_buffer.h"
#include <stdlib.h>

struct GapBuffer {
    int dummy;
};

GapBuffer* gb_create(size_t initial_capacity) {
    (void)initial_capacity;
    GapBuffer* gb = (GapBuffer*)malloc(sizeof(GapBuffer));
    if (gb) gb->dummy = 0;
    return gb;
}

void gb_destroy(GapBuffer* gb) {
    free(gb);
}

int gb_insert(GapBuffer* gb, const char* text, size_t len) {
    (void)gb; (void)text; (void)len;
    return 0;
}

int gb_delete(GapBuffer* gb, size_t count, int forward) {
    (void)gb; (void)count; (void)forward;
    return 0;
}

const char* gb_get_text(const GapBuffer* gb) {
    (void)gb;
    return "";
}

size_t gb_length(const GapBuffer* gb) {
    (void)gb;
    return 0;
}