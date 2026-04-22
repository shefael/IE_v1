#include "utils/config.h"
#include <stdlib.h>

void* config_load(const char* path) {
    (void)path;
    int* dummy = (int*)malloc(sizeof(int));
    if (dummy) *dummy = 0;
    return dummy;
}

int config_save(const char* path, const void* config) {
    (void)path; (void)config;
    return 0;
}

void config_free(void* config) {
    free(config);
}