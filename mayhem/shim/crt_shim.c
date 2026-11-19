/* Linux implementations of the MSVC secure-CRT calls the harnessed Catime sources use.
 * Native (32-bit) wchar_t: do not link this into -fshort-wchar targets. */
#include "windows.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static errno_t copy_w(wchar_t* dst, size_t size, const wchar_t* src, size_t count) {
    if (!dst || size == 0) abort();
    if (!src) { dst[0] = 0; abort(); }
    size_t n = 0;
    while (n < count && src[n]) n++;
    if (n >= size) {
        if (count == _TRUNCATE) { n = size - 1; memcpy(dst, src, n * sizeof(wchar_t)); dst[n] = 0; return 80; /* STRUNCATE */ }
        dst[0] = 0;
        abort(); /* MSVC: invalid parameter handler terminates the process */
    }
    memcpy(dst, src, n * sizeof(wchar_t));
    dst[n] = 0;
    return 0;
}
errno_t wcscpy_s(wchar_t* dst, size_t size, const wchar_t* src) { return copy_w(dst, size, src, (size_t)-2); }
errno_t wcsncpy_s(wchar_t* dst, size_t size, const wchar_t* src, size_t count) { return copy_w(dst, size, src, count); }
errno_t wcscat_s(wchar_t* dst, size_t size, const wchar_t* src) {
    if (!dst || size == 0) abort();
    size_t l = 0;
    while (l < size && dst[l]) l++;
    if (l == size) abort();
    return copy_w(dst + l, size - l, src, (size_t)-2);
}
static errno_t copy_a(char* dst, size_t size, const char* src, size_t count) {
    if (!dst || size == 0) abort();
    if (!src) { dst[0] = 0; abort(); }
    size_t n = 0;
    while (n < count && src[n]) n++;
    if (n >= size) {
        if (count == _TRUNCATE) { n = size - 1; memcpy(dst, src, n); dst[n] = 0; return 80; }
        dst[0] = 0;
        abort();
    }
    memcpy(dst, src, n);
    dst[n] = 0;
    return 0;
}
errno_t strcpy_s(char* dst, size_t size, const char* src) { return copy_a(dst, size, src, (size_t)-2); }
errno_t strncpy_s(char* dst, size_t size, const char* src, size_t count) { return copy_a(dst, size, src, count); }

int _snwprintf_s(wchar_t* buf, size_t size, size_t count, const wchar_t* fmt, ...) {
    if (!buf || size == 0) abort();
    size_t lim = (count == _TRUNCATE || count >= size) ? size : count + 1;
    va_list ap;
    va_start(ap, fmt);
    int r = vswprintf(buf, lim, fmt, ap);
    va_end(ap);
    if (r < 0) { buf[lim - 1] = 0; if (count != _TRUNCATE) { buf[0] = 0; abort(); } return -1; }
    return r;
}

HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR op, LPCWSTR file, LPCWSTR params, LPCWSTR dir, int show) {
    (void)hwnd; (void)op; (void)file; (void)params; (void)dir; (void)show;
    return NULL; /* never launch anything from a harness */
}

/* strsafe: copy with truncation, report INSUFFICIENT_BUFFER (dst always terminated). */
#include "strsafe.h"
HRESULT StringCchCopyA(char* dst, size_t cch, const char* src) {
    if (!dst || cch == 0 || !src) return STRSAFE_E_INVALID_PARAMETER;
    size_t n = 0;
    while (n < cch - 1 && src[n]) { dst[n] = src[n]; n++; }
    dst[n] = 0;
    return src[n] ? STRSAFE_E_INSUFFICIENT_BUFFER : S_OK;
}
HRESULT StringCbCopyA(char* dst, size_t cb, const char* src) { return StringCchCopyA(dst, cb, src); }
HRESULT StringCchCopyW(wchar_t* dst, size_t cch, const wchar_t* src) {
    if (!dst || cch == 0 || !src) return STRSAFE_E_INVALID_PARAMETER;
    size_t n = 0;
    while (n < cch - 1 && src[n]) { dst[n] = src[n]; n++; }
    dst[n] = 0;
    return src[n] ? STRSAFE_E_INSUFFICIENT_BUFFER : S_OK;
}

/* include/log.h: logging goes nowhere in the harnesses. */
#include "log.h"
void WriteLog(LogLevel level, const char* format, ...) { (void)level; (void)format; }
