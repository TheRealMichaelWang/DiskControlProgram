#include <stdint.h>

#ifndef FS_H
#define FS_H

typedef struct fs_entry
{
	char name[8];
	char ext[3];
	uint32_t size;
	uint32_t lba;
	uint32_t parent_dir;
	uint8_t type;
	uint32_t children[32];
	uint8_t children_count;
}fs_entry;

#define MOUNT_STATUS_MOUNTED 1
#define MOUNT_STATUS_UNMOUNTED 0

uint32_t getMountStatus();
uint32_t getCurrentDrive();


void fs_mount(uint8_t drive);
void fs_eject();

fs_entry* fs_search(char path[]);
void fs_print_dir_listing(char path[]);
fs_entry* fs_make_dir(char path[], char name[]);
fs_entry* fs_make_file(char path[], char name[], char ext[], uint32_t size);

void dbg_print_all();
void fs_print_entry(fs_entry entry);
#endif // !FS_H
