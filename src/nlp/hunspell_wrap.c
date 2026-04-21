#include "nlp/hunspell_wrap.h"
#include <stdlib.h>

int spell_init(const char* aff_path, const char* dic_path) {
    (void)aff_path; (void)dic_path;
    return 0;
}

int spell_check(const char* word) {
    (void)word;
    return 1;
}

char** spell_suggest(const char* word, int* nsuggest) {
    (void)word;
    *nsuggest = 0;
    return NULL;
}

void spell_cleanup(void) {
}