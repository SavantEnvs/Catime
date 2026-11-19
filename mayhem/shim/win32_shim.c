/* POSIX/portable implementations of the few Win32 calls the harnessed Catime code makes. */
#include "windows.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

static int fd_of(HANDLE h) { return (int)(intptr_t)h - 1; }

/* Encode one code point into dst as wchar_t units; returns units written (needed). */
static int put_wide(uint32_t cp, wchar_t* out) {
    if (sizeof(wchar_t) == 2 && cp >= 0x10000) {
        cp -= 0x10000;
        if (out) { out[0] = (wchar_t)(0xD800 + (cp >> 10)); out[1] = (wchar_t)(0xDC00 + (cp & 0x3FF)); }
        return 2;
    }
    if (out) out[0] = (wchar_t)cp;
    return 1;
}

int MultiByteToWideChar(UINT cp, DWORD flags, LPCSTR src, int srcLen, LPWSTR dst, int dstLen) {
    (void)cp; (void)flags;
    if (!src) return 0;
    size_t n = srcLen < 0 ? strlen(src) + 1 : (size_t)srcLen;
    const unsigned char* s = (const unsigned char*)src;
    int written = 0;
    size_t i = 0;
    while (i < n) {
        uint32_t c = s[i];
        int extra = 0;
        if (c < 0x80) extra = 0;
        else if ((c & 0xE0) == 0xC0) { c &= 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { c &= 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { c &= 0x07; extra = 3; }
        else { c = 0xFFFD; extra = 0; }
        i++;
        for (int k = 0; k < extra; k++) {
            if (i >= n || (s[i] & 0xC0) != 0x80) { c = 0xFFFD; break; }
            c = (c << 6) | (s[i] & 0x3F);
            i++;
        }
        if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) c = 0xFFFD;
        wchar_t tmp[2];
        int u = put_wide(c, tmp);
        if (dstLen > 0) {
            if (written + u > dstLen) return 0; /* ERROR_INSUFFICIENT_BUFFER */
            memcpy(dst + written, tmp, (size_t)u * sizeof(wchar_t));
        }
        written += u;
    }
    return written;
}

int WideCharToMultiByte(UINT cp, DWORD flags, LPCWSTR src, int srcLen, LPSTR dst, int dstLen,
                        LPCSTR defChar, BOOL* usedDef) {
    (void)cp; (void)flags; (void)defChar;
    if (usedDef) *usedDef = FALSE;
    if (!src) return 0;
    size_t n = (size_t)srcLen;
    if (srcLen < 0) { n = 0; while (src[n]) n++; n++; } /* no wcslen: wchar_t may be 16-bit */
    int written = 0;
    for (size_t i = 0; i < n; i++) {
        uint32_t c = (uint32_t)src[i];
        if (sizeof(wchar_t) == 2) c &= 0xFFFF;
        if (sizeof(wchar_t) == 2 && c >= 0xD800 && c <= 0xDBFF && i + 1 < n &&
            ((uint32_t)src[i + 1] & 0xFC00) == 0xDC00) {
            c = 0x10000 + ((c - 0xD800) << 10) + (((uint32_t)src[i + 1] & 0xFFFF) - 0xDC00);
            i++;
        }
        if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) c = 0xFFFD;
        unsigned char b[4];
        int u;
        if (c < 0x80) { b[0] = (unsigned char)c; u = 1; }
        else if (c < 0x800) { b[0] = 0xC0 | (c >> 6); b[1] = 0x80 | (c & 0x3F); u = 2; }
        else if (c < 0x10000) { b[0] = 0xE0 | (c >> 12); b[1] = 0x80 | ((c >> 6) & 0x3F); b[2] = 0x80 | (c & 0x3F); u = 3; }
        else { b[0] = 0xF0 | (c >> 18); b[1] = 0x80 | ((c >> 12) & 0x3F); b[2] = 0x80 | ((c >> 6) & 0x3F); b[3] = 0x80 | (c & 0x3F); u = 4; }
        if (dstLen > 0) {
            if (written + u > dstLen) return 0;
            memcpy(dst + written, b, (size_t)u);
        }
        written += u;
    }
    return written;
}

HANDLE CreateFileW(LPCWSTR path, DWORD access, DWORD share, void* sa, DWORD disp, DWORD flags, HANDLE tmpl) {
    (void)access; (void)share; (void)sa; (void)disp; (void)flags; (void)tmpl;
    char p[4 * MAX_PATH];
    if (WideCharToMultiByte(CP_UTF8, 0, path, -1, p, (int)sizeof p, NULL, NULL) <= 0)
        return INVALID_HANDLE_VALUE;
    int fd = open(p, O_RDONLY);
    if (fd < 0) return INVALID_HANDLE_VALUE;
    return (HANDLE)(intptr_t)(fd + 1);
}

BOOL ReadFile(HANDLE h, void* buf, DWORD n, DWORD* nread, void* ov) {
    (void)ov;
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd_of(h), (char*)buf + got, n - got);
        if (r < 0) return FALSE;
        if (r == 0) break;
        got += (size_t)r;
    }
    if (nread) *nread = (DWORD)got;
    return TRUE;
}

BOOL SetFilePointerEx(HANDLE h, LARGE_INTEGER dist, LARGE_INTEGER* newpos, DWORD method) {
    int whence = method == 0 ? SEEK_SET : method == 1 ? SEEK_CUR : SEEK_END;
    off_t r = lseek(fd_of(h), (off_t)dist.QuadPart, whence);
    if (r < 0) return FALSE;
    if (newpos) newpos->QuadPart = r;
    return TRUE;
}

BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER* size) {
    struct stat st;
    if (fstat(fd_of(h), &st) != 0) return FALSE;
    size->QuadPart = st.st_size;
    return TRUE;
}

BOOL CloseHandle(HANDLE h) { return close(fd_of(h)) == 0; }
