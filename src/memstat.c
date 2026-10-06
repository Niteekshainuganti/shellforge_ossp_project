#include "memstat.h"

#include <stdio.h>
#include <string.h>

void memstat_print(void) {
    FILE *fp = fopen("/proc/self/status", "r");

    if (!fp) {
        perror("shellforge: memstat");
        return;
    }

    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmPeak:", 7) == 0) {
            fputs(line, stdout);
        }
    }

    fclose(fp);
}
