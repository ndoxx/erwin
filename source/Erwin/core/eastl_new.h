#pragma once

// void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) 
void* operator new[](size_t size, const char*, int, unsigned, const char*, int) 
{
  return malloc(size);
}  

// void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line) 
void* operator new[](size_t size, size_t, size_t, const char*, int, unsigned, const char*, int) 
{
  return malloc(size);
}