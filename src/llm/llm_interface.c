#include "llm/llm_interface.h"

int llm_init(const char* model_path) {
    (void)model_path;
    return 0;
}

void llm_cleanup(void) {
}

const char* llm_generate(const char* prompt) {
    (void)prompt;
    return "";
}