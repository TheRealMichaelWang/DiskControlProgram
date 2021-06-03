#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

void memory_copy(void* source, void* dest, int length);
void memory_fill(void* ptr, uint8_t byte, int length);
void* malloc(uint32_t size);
int free(void* ptr);
#endif