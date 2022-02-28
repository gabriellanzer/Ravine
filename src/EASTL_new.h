#ifndef EASTL_NEW_H
#define EASTL_NEW_H

#include "EASTL/initializer_list.h"
#include "EASTL/numeric.h"

void* __cdecl operator new[](size_t size, size_t a, size_t b, const char* name, int flags, unsigned int debugFlags,
			     const char* file, int line);
void* __cdecl operator new[](size_t size, const char* name, int flags, unsigned int debugFlags, const char* file,
			     int line);

int __cdecl Vsnprintf8(char* pDestination, size_t n, const char* pFormat, va_list arguments);
int __cdecl Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments);
#endif