/* Known-answer check for DecodeAniCursor(): prints "<basename> ok=<0|1> frames=<n> delays=<ms,...>"
 * for each .ani given; test.sh diffs it against ani.expected. Links the harness's
 * DecodedAnimation_Free (mayhem/harnesses/fuzz_ani.c) via -DANI_CHECK_NO_FUZZ_ENTRY. */
#include <stdio.h>
#include <string.h>
#include "tray/tray_animation_decoder.h"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        const char* base = strrchr(argv[i], '/');
        base = base ? base + 1 : argv[i];
        DecodedAnimation a;
        memset(&a, 0, sizeof a);
        BOOL ok = DecodeAniCursor(argv[i], &a, 32, 32);
        printf("%s ok=%d frames=%d delays=", base, ok ? 1 : 0, a.count);
        for (int k = 0; k < a.count; k++) printf("%s%u", k ? "," : "", a.delays[k]);
        printf(" animated=%d\n", a.isAnimated ? 1 : 0);
        DecodedAnimation_Free(&a);
    }
    return 0;
}
