/* Fuzz Catime's colour/gradient string parsing (config.ini values, plugin <color> tags):
 * isValidColor, isValidColorOrGradient, normalizeColor, ColorStringToColorRef, gradient lookup. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "color/color_parser.h"
#include "color/gradient.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    char* s = (char*)malloc(size + 1);
    if (!s) return 0;
    memcpy(s, data, size);
    s[size] = '\0';
    char out[256];
    COLORREF c;
    (void)isValidColor(s);
    (void)isValidColorOrGradient(s);
    normalizeColor(s, out, sizeof out);
    (void)ColorStringToColorRef(s, &c);
    ReplaceBlackColor(s, out, sizeof out);
    GradientInfoSnapshot snap;
    if (GetGradientInfoSnapshotByName(s, &snap) != GRADIENT_NONE) {
        (void)GetGradientColorAt(&snap.info, 0.5f);
    }
    free(s);
    return 0;
}
