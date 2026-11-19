/* Minimal Win32 shim so Catime's portable parser sources compile natively on Linux
 * for fuzzing. Only the types/macros/functions the harnessed files use. */
#ifndef CATIME_MAYHEM_WINDOWS_SHIM_H
#define CATIME_MAYHEM_WINDOWS_SHIM_H
#include <stddef.h>
#include <stdlib.h> /* the real windows.h drags these in too */
#include <stdio.h>
#include <stdint.h>
#include <wchar.h>
#include <string.h>
#include <strings.h>

typedef int BOOL;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef unsigned int UINT;
typedef int32_t LONG;
typedef int64_t LONGLONG;
typedef wchar_t WCHAR;
typedef char CHAR;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;
typedef void* HANDLE;
typedef void* HWND;
typedef void* HDC;
typedef void* HICON;
typedef void* HINSTANCE;
typedef void* LPVOID;
typedef DWORD COLORREF;
typedef union { struct { DWORD LowPart; LONG HighPart; } u; LONGLONG QuadPart; } LARGE_INTEGER;
typedef struct { LONG left, top, right, bottom; } RECT;
typedef struct { LONG x, y; } POINT;
typedef struct { void* Ptr; } SRWLOCK;
#define SRWLOCK_INIT {0}

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define MAX_PATH 260
#define MAXDWORD 0xffffffffu
#define WINAPI
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | ((WORD)((BYTE)(g)) << 8) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(c) ((BYTE)(c))
#define GetGValue(c) ((BYTE)(((WORD)(c)) >> 8))
#define GetBValue(c) ((BYTE)((c) >> 16))
#define ZeroMemory(p, n) memset((p), 0, (n))
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define strtok_s strtok_r
#define wcstok_s wcstok

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define GENERIC_READ 0x80000000u
#define FILE_SHARE_READ 1u
#define OPEN_EXISTING 3u
#define FILE_ATTRIBUTE_NORMAL 0x80u
#define FILE_BEGIN 0u
#define CP_UTF8 65001u

/* Single-threaded harnesses: locks are no-ops. */
static inline void AcquireSRWLockExclusive(SRWLOCK* l) { (void)l; }
static inline void ReleaseSRWLockExclusive(SRWLOCK* l) { (void)l; }
static inline void AcquireSRWLockShared(SRWLOCK* l) { (void)l; }
static inline void ReleaseSRWLockShared(SRWLOCK* l) { (void)l; }

/* POSIX-backed file API + UTF-8 <-> wchar_t (UTF-16 when -fshort-wchar, else UTF-32). */
HANDLE CreateFileW(LPCWSTR path, DWORD access, DWORD share, void* sa, DWORD disp, DWORD flags, HANDLE tmpl);
BOOL ReadFile(HANDLE h, void* buf, DWORD n, DWORD* read, void* ov);
BOOL SetFilePointerEx(HANDLE h, LARGE_INTEGER dist, LARGE_INTEGER* newpos, DWORD method);
BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER* size);
BOOL CloseHandle(HANDLE h);
int MultiByteToWideChar(UINT cp, DWORD flags, LPCSTR src, int srcLen, LPWSTR dst, int dstLen);
int WideCharToMultiByte(UINT cp, DWORD flags, LPCWSTR src, int srcLen, LPSTR dst, int dstLen,
                        LPCSTR defChar, BOOL* usedDef);

/* MSVC secure-CRT subset (mayhem/shim/crt_shim.c). As with the MSVC default invalid-parameter
 * handler, a destination too small for a non-_TRUNCATE copy terminates the process (abort). */
#define _TRUNCATE ((size_t)-1)
typedef int errno_t;
errno_t wcscpy_s(wchar_t* dst, size_t size, const wchar_t* src);
errno_t wcsncpy_s(wchar_t* dst, size_t size, const wchar_t* src, size_t count);
errno_t wcscat_s(wchar_t* dst, size_t size, const wchar_t* src);
errno_t strcpy_s(char* dst, size_t size, const char* src);
errno_t strncpy_s(char* dst, size_t size, const char* src, size_t count);
int _snwprintf_s(wchar_t* buf, size_t size, size_t count, const wchar_t* fmt, ...);
#define _wcsicmp wcscasecmp
#define _wcsnicmp wcsncasecmp
#define SW_SHOWNORMAL 1
HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR op, LPCWSTR file, LPCWSTR params, LPCWSTR dir, int show);
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#define UNREFERENCED_PARAMETER(p) ((void)(p))
#define _wcsdup wcsdup
#define _strdup strdup
static inline BOOL PtInRect(const RECT* r, POINT p) {
    return p.x >= r->left && p.x < r->right && p.y >= r->top && p.y < r->bottom;
}
typedef intptr_t LRESULT;
typedef uintptr_t WPARAM;
typedef uintptr_t UINT_PTR;
typedef uintptr_t DWORD_PTR;
typedef intptr_t LPARAM;
#define CALLBACK

/* Icon/cursor subset (mayhem/shim/gdi_shim.c): handles are heap tokens, so a leaked or
 * double-destroyed icon shows up under ASan like a real handle bug would. */
typedef size_t SIZE_T;
typedef BYTE* PBYTE;
typedef void* HCURSOR;
typedef void* HBITMAP;
typedef void* HGDIOBJ;
typedef struct { BOOL fIcon; DWORD xHotspot; DWORD yHotspot; HBITMAP hbmMask; HBITMAP hbmColor; } ICONINFO;
#define LR_DEFAULTCOLOR 0u
#define WAIT_OBJECT_0 0u
#define WAIT_TIMEOUT 258u
DWORD WaitForSingleObject(HANDLE h, DWORD ms);
HICON CreateIconFromResourceEx(PBYTE bits, DWORD size, BOOL icon, DWORD ver, int cx, int cy, UINT flags);
BOOL GetIconInfo(HICON icon, ICONINFO* info);
HICON CreateIconIndirect(ICONINFO* info);
BOOL DeleteObject(HGDIOBJ obj);
BOOL DestroyIcon(HICON icon);
HICON CopyIcon(HICON icon);
BOOL DestroyCursor(HCURSOR cursor);
#endif
