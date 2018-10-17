#pragma once

#if defined(__linux__)

#include <cstdio>
#include <cstdlib>

#include <execinfo.h>


// see https://www.gnu.org/software/libc/manual/html_node/Backtraces.html
inline void print_trace() {
    void *array[10];
    size_t size;
    char **strings;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    printf("Obtained %zd stack frames.\n", size);
    for(size_t i = 0; i < size; i++) {
        printf("%s\n", strings[i]);
    }

    free(strings);
}

#else

void print_trace() {}

#endif

