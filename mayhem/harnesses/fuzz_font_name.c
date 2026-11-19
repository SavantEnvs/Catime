/* Fuzz Catime's own TTF/OTF 'name' table parser through its public entry point
 * GetFontNameFromFile() (include/font/font_ttf_parser.h), which the app calls on every
 * user-supplied font file. Built with -fshort-wchar so WCHAR/wchar_t is UTF-16 as on Windows. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "font/font_ttf_parser.h"

static char g_path[512];

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (!g_path[0]) {
        const char* tmp = getenv("TMPDIR");
        snprintf(g_path, sizeof g_path, "%s/catime_font_XXXXXX", (tmp && *tmp) ? tmp : "/tmp");
        int fd = mkstemp(g_path);
        if (fd < 0) abort();
        close(fd);
    }
    FILE* f = fopen(g_path, "wb");
    if (!f) abort();
    if (size && fwrite(data, 1, size, f) != size) abort();
    fclose(f);
    char name[256];
    (void)GetFontNameFromFile(g_path, name, sizeof name);
    return 0;
}
