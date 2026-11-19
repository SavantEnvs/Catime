/* Icon/cursor stand-ins for the ANI decoder harness. CreateIconFromResourceEx reads the whole
 * resource range it is given (as user32 would parse it), so an out-of-range blob the decoder
 * hands over is caught by ASan; it "succeeds" for any plausible-size resource. */
#include "windows.h"
#include <stdlib.h>

volatile unsigned g_gdi_shim_sink;

DWORD WaitForSingleObject(HANDLE h, DWORD ms) { (void)h; (void)ms; return WAIT_TIMEOUT; }

static HICON new_handle(void) { return (HICON)malloc(1); }

HICON CreateIconFromResourceEx(PBYTE bits, DWORD size, BOOL icon, DWORD ver, int cx, int cy, UINT flags) {
    (void)icon; (void)ver; (void)cx; (void)cy; (void)flags;
    if (!bits || size < 40) return NULL; /* smaller than a BITMAPINFOHEADER: user32 rejects it */
    unsigned acc = 0;
    for (DWORD i = 0; i < size; i++) acc += bits[i];
    g_gdi_shim_sink += acc;
    return new_handle();
}
BOOL GetIconInfo(HICON icon, ICONINFO* info) {
    if (!icon || !info) return FALSE;
    memset(info, 0, sizeof *info);
    return TRUE;
}
HICON CreateIconIndirect(ICONINFO* info) { return info ? new_handle() : NULL; }
BOOL DeleteObject(HGDIOBJ obj) { (void)obj; return TRUE; }
BOOL DestroyIcon(HICON icon) { free(icon); return icon != NULL; }
BOOL DestroyCursor(HCURSOR cursor) { free(cursor); return cursor != NULL; }
HICON CopyIcon(HICON icon) { return icon ? new_handle() : NULL; }
