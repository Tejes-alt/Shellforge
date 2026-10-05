#define _POSIX_C_SOURCE 200809L
#include "shellforge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *sf_strdup(const char *s) {
    if (!s) return NULL;
    char *p = strdup(s);
    if (!p) { perror("strdup"); exit(EXIT_FAILURE); }
    return p;
}
void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1]=='\n' || s[n-1]=='\r')) s[--n] = '\0';
}
