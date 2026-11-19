/* strsafe.h subset for the Catime harness shim. */
#ifndef CATIME_MAYHEM_STRSAFE_SHIM_H
#define CATIME_MAYHEM_STRSAFE_SHIM_H
#include "windows.h"
typedef long HRESULT;
#define S_OK ((HRESULT)0)
#define STRSAFE_E_INSUFFICIENT_BUFFER ((HRESULT)0x8007007AL)
#define STRSAFE_E_INVALID_PARAMETER ((HRESULT)0x80070057L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
HRESULT StringCbCopyA(char* dst, size_t cb, const char* src);
HRESULT StringCchCopyA(char* dst, size_t cch, const char* src);
HRESULT StringCchCopyW(wchar_t* dst, size_t cch, const wchar_t* src);
#endif
