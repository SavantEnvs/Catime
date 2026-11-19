/* Known-answer check for GetFontNameFromFile(): prints "<basename>\t<family name>" for each
 * font given on the command line (FAILED when the parser rejects it). test.sh diffs the
 * output against font_names.expected. */
#include <stdio.h>
#include <string.h>
#include "font/font_ttf_parser.h"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        char name[256];
        const char* base = strrchr(argv[i], '/');
        base = base ? base + 1 : argv[i];
        if (GetFontNameFromFile(argv[i], name, sizeof name)) printf("%s\t%s\n", base, name);
        else printf("%s\tFAILED\n", base);
    }
    return 0;
}
