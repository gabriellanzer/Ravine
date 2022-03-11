#include "EASTL_new.h"
#include "fmt/core.h"
#include "fmt/printf.h"

void* __cdecl operator new[](size_t size, size_t a, size_t b, const char* name, int flags, unsigned int debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}
void* __cdecl operator new[](size_t size, const char* name, int flags, unsigned int debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}

// EASTL also wants us to define this (see string.h line 197)
int __cdecl Vsnprintf8(char* pDestination, size_t n, const char* pFormat, va_list arguments)
{
    #ifdef _MSC_VER
        return _vsnprintf_s(pDestination, n, n, pFormat, arguments);
    #else
        return vsnprintf(pDestination, n, pFormat, arguments);
    #endif
}

int __cdecl Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
{
    #ifdef _MSC_VER
        return _vsnwprintf_s((wchar_t*)pDestination, n, n, (wchar_t*)pFormat, arguments);
    #else
		char* d = new char[n+1];
		int r = vsnprintf(d, n, convertstring<char16_t, char>(pFormat).c_str(), arguments);
		memcpy(pDestination, convertstring<char, char16_t>(d).c_str(), (n+1)*sizeof(char16_t));
		delete[] d;
		return r;
    #endif
}