#include "mem.h"
#include "string.h"
#include "../drivers/screen.h"

void memory_copy(void *source, void *dest, int length) {
    int i;
    uint8_t* source_bytes = (uint8_t*)source;
    uint8_t* dest_bytes = (uint8_t*)dest;
    for (i = 0; i < length; i++) {
        *(dest_bytes + i) = *(source_bytes + i);
    }
}

void memory_fill(void* ptr, uint8_t byte, int length)
{
    uint8_t* source_bytes = (uint8_t*)ptr;
    for (int i = 0; i < length; i++)
    {
        *(source_bytes + i) = byte;
    }
}

typedef struct memory_block
{
    void* pointer;
    uint32_t size;
    uint8_t status;
} memory_block;

#define MEM_BLOCK_STATUS_IN_USE 1
#define MEM_BLOCK_STATUS_FREE 0

#define FREE_MEMORY_START 0x10000

static memory_block blocks[1000];
static int allocated_blocks = 0;

static uint32_t free_memory_addr = FREE_MEMORY_START;

void* malloc(uint32_t size)
{
    for (int i = 0; i < allocated_blocks; i++)
    {
        if (size <= blocks[i].size && blocks[i].status == MEM_BLOCK_STATUS_FREE)
        {
            memory_fill(blocks[i].pointer, 0, blocks[i].size);
            blocks[i].status = MEM_BLOCK_STATUS_IN_USE;
            return blocks[i].pointer;
        }
    }
    blocks[allocated_blocks].status = MEM_BLOCK_STATUS_IN_USE;
    blocks[allocated_blocks].size = size;
    blocks[allocated_blocks].pointer = (void*)free_memory_addr;
    free_memory_addr = free_memory_addr + size;
    return blocks[allocated_blocks++].pointer;
}

int free(void* ptr)
{
    for (int i = 0; i < allocated_blocks; i++)
    {
        if (blocks[i].pointer == ptr && blocks[i].status == MEM_BLOCK_STATUS_IN_USE)
        {
            blocks[i].status = MEM_BLOCK_STATUS_FREE;
            return 0;
        }
    }
    return 1;
}