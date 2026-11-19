/* Fuzz the vendored (upstream-patched) stb_truetype exactly as Catime's STB text renderer
 * drives it (src/drawing/drawing_text_stb_*.c): offset -> InitFont -> metrics -> glyph
 * lookup/kerning -> shape -> Rasterize. Input: a font file (TTF/OTF/TTC). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

static const int kCodepoints[] = {'0', '1', '9', ':', 'A', 'g', ' ', 0x00E9, 0x4E2D, 0x1F600};

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > (1u << 22)) return -1;
    /* Catime hands stb_truetype a MapViewOfFile() view of the font (LoadFontMappingW in
     * src/drawing/drawing_text_stb_font_cache.c) and never passes a length: stb "does no range
     * checking of the offsets found in the file" (stb_truetype.h:8). A file mapping is
     * page-granular and zero-filled past EOF, so model exactly that: round the buffer up to a
     * 4 KiB page and zero the tail. Reads beyond the mapped pages are still caught by ASan. */
    size_t mapped = (size + 4095) & ~(size_t)4095;
    if (mapped == 0) mapped = 4096;
    unsigned char* buf = (unsigned char*)calloc(mapped, 1);
    if (!buf) return 0;
    memcpy(buf, data, size);
    stbtt_fontinfo font;
    int offset = stbtt_GetFontOffsetForIndex(buf, 0);
    if (offset < 0 || !stbtt_InitFont(&font, buf, offset)) { free(buf); return 0; }

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    static const float heights[] = {12.0f, 48.0f};
    for (size_t h = 0; h < sizeof heights / sizeof heights[0]; h++) {
        float scale = stbtt_ScaleForPixelHeight(&font, heights[h]);
        int prev = 0;
        for (size_t i = 0; i < sizeof kCodepoints / sizeof kCodepoints[0]; i++) {
            int g = stbtt_FindGlyphIndex(&font, kCodepoints[i]);
            int adv, lsb;
            stbtt_GetGlyphHMetrics(&font, g, &adv, &lsb);
            if (prev) (void)stbtt_GetGlyphKernAdvance(&font, prev, g);
            prev = g;
            int x0, y0, x1, y1;
            stbtt_GetGlyphBitmapBox(&font, g, scale, scale, &x0, &y0, &x1, &y1);
            int w = x1 - x0, hh = y1 - y0;
            if (w <= 0 || hh <= 0 || w > 512 || hh > 512) continue;
            stbtt_vertex* verts = NULL;
            int n = stbtt_GetGlyphShape(&font, g, &verts);
            if (verts && n > 0) {
                unsigned char* px = (unsigned char*)calloc((size_t)w * hh, 1);
                if (px) {
                    stbtt__bitmap gbm;
                    gbm.w = w; gbm.h = hh; gbm.stride = w; gbm.pixels = px;
                    stbtt_Rasterize(&gbm, 0.35f, verts, n, scale, scale, 0.0f, 0.0f, x0, y0, 1, font.userdata);
                    free(px);
                }
            }
            if (verts) stbtt_FreeShape(&font, verts);
        }
    }
    free(buf);
    return 0;
}
