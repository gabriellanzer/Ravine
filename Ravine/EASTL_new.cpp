#include "EASTL_new.h"
#include <eastl/numeric.h>

void* __cdecl operator new[](size_t size, size_t a, size_t b, const char* name, int flags, unsigned int debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}
void* __cdecl operator new[](size_t size, const char* name, int flags, unsigned int debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}