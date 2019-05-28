#ifndef EASTL_NEW_H
#define EASTL_NEW_H

void* __cdecl operator new[](size_t size, size_t a, size_t b, const char* name, int flags, unsigned int debugFlags, const char* file, int line);
void* __cdecl operator new[](size_t size, const char* name, int flags, unsigned int debugFlags, const char* file, int line);

#endif