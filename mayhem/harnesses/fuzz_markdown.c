/* Fuzz Catime's markdown / rich-text parser (plugin output, notifications, about/markdown
 * dialogs) through its public entry point ParseMarkdownLinks() (include/markdown/
 * markdown_parser.h): links, headings, styles, lists, blockquotes, <color>/<font> tags.
 * Input bytes are UTF-8, converted to wchar_t like the app does (Utf8ToWideAlloc). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "markdown/markdown_parser.h"
#include "utils/string_convert.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > 65536) return -1;
    char* s = (char*)malloc(size + 1);
    if (!s) return 0;
    memcpy(s, data, size);
    s[size] = '\0';
    wchar_t* w = Utf8ToWideAlloc(s);
    free(s);
    if (!w) return 0;

    wchar_t* display = NULL;
    MarkdownLink* links = NULL; int nLinks = 0;
    MarkdownHeading* headings = NULL; int nHead = 0;
    MarkdownStyle* styles = NULL; int nStyle = 0;
    MarkdownListItem* items = NULL; int nItems = 0;
    MarkdownBlockquote* quotes = NULL; int nQuotes = 0;
    MarkdownColorTag* colors = NULL; int nColors = 0;
    MarkdownFontTag* fonts = NULL; int nFonts = 0;
    if (ParseMarkdownLinks(w, &display, &links, &nLinks, &headings, &nHead, &styles, &nStyle,
                           &items, &nItems, &quotes, &nQuotes, &colors, &nColors, &fonts, &nFonts)) {
        int len = display ? (int)wcslen(display) : 0;
        for (int pos = 0; pos <= len && pos < 4096; pos++) {
            int idx = -1;
            (void)IsCharacterInLink(links, nLinks, pos, &idx);
            (void)IsCharacterInHeading(headings, nHead, pos, &idx);
            (void)IsCharacterInStyle(styles, nStyle, pos, &idx);
            (void)IsCharacterInListItem(items, nItems, pos, &idx);
            (void)IsCharacterInBlockquote(quotes, nQuotes, pos, &idx);
            (void)IsCharacterInFontTag(fonts, nFonts, pos, &idx);
            if (IsCharacterInColorTag(colors, nColors, pos, &idx) && idx >= 0 && idx < nColors)
                (void)InterpolateGradientColor(&colors[idx], pos);
        }
        POINT pt = {0, 0};
        (void)GetClickedLinkUrl(links, nLinks, pt);
    }
    FreeMarkdownLinks(links, nLinks);
    free(display); free(headings); free(styles); free(items); free(quotes); free(colors); free(fonts);
    free(w);
    return 0;
}
