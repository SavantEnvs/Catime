/* Fuzz Catime's .ani (RIFF ACON) tray-animation decoder through its public entry point
 * DecodeAniCursor() (include/tray/tray_animation_decoder.h): RIFF chunk/LIST walking,
 * anih/rate/seq metadata, and ICO/CUR directory entry selection per frame. Users drop
 * arbitrary .ani files into the animations folder. Icon creation is a shim (gdi_shim.c). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tray/tray_animation_decoder.h"

static char g_path[512];

/* DecodedAnimation_Free() lives in src/tray/tray_animation_decoder.c next to the WIC decoders,
 * which do not build off Windows; this is a verbatim copy of its body. */
void DecodedAnimation_Free(DecodedAnimation* a) {
    if (!a) return;
    if (a->icons) {
        for (int i = 0; i < a->count; ++i) if (a->icons[i]) DestroyIcon(a->icons[i]);
        free(a->icons);
    }
    free(a->delays);
    free(a->canvas);
    memset(a, 0, sizeof *a);
}

#ifndef ANI_CHECK_NO_FUZZ_ENTRY
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (!g_path[0]) {
        const char* tmp = getenv("TMPDIR");
        snprintf(g_path, sizeof g_path, "%s/catime_ani_XXXXXX", (tmp && *tmp) ? tmp : "/tmp");
        int fd = mkstemp(g_path);
        if (fd < 0) abort();
        close(fd);
    }
    FILE* f = fopen(g_path, "wb");
    if (!f) abort();
    if (size && fwrite(data, 1, size, f) != size) abort();
    fclose(f);
    DecodedAnimation anim;
    memset(&anim, 0, sizeof anim);
    if (DecodeAniCursor(g_path, &anim, 32, 32)) {
        for (int i = 0; i < anim.count; i++) (void)anim.delays[i];
    }
    DecodedAnimation_Free(&anim);
    return 0;
}
#endif
