/* Fuzz Catime's countdown time-expression parser (CLI args / input dialog / config):
 * TimeParser_Validate, HasUnits, ParseBasic, ParseAdvanced, then round-trip formatting. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils/time_parser.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    char* s = (char*)malloc(size + 1);
    if (!s) return 0;
    memcpy(s, data, size);
    s[size] = '\0';
    int secs = 0;
    (void)TimeParser_Validate(s);
    (void)TimeParser_HasUnits(s);
    if (TimeParser_ParseBasic(s, &secs)) {
        char out[64];
        TimeParser_FormatToString(secs, out, sizeof out);
    }
    if (TimeParser_ParseAdvanced(s, &secs)) {
        int h, m, sec;
        TimeParser_FormatToHMS(secs, &h, &m, &sec);
    }
    free(s);
    return 0;
}
