#include "fs.h"
#include "io.h"
#include "../drivers/disk.h"
#include "../libc/mem.h"
#include "../libc/string.h"

#define MAGIC_NUMBER 0x21878

#define FS_TYPE_FILE 0
#define FS_TYPE_DIR 1
#define FS_TYPE_ROOT 2


static uint8_t fs_mount_status = MOUNT_STATUS_UNMOUNTED;
static uint8_t fs_current_drive;
static fs_entry entries[1000];
static uint32_t entry_count;


fs_entry* fs_add_entry(fs_entry entry);
void save_fs_data(uint32_t magic_number);
fs_entry* fs_search(char path[]);
uint32_t getIndex(fs_entry* entry);

uint32_t size_in_sectors(uint32_t size)
{
	return ((size - (size % 512)) / 512) + 1;
}

uint32_t getMountStatus()
{
	return fs_mount_status;
}

uint32_t getCurrentDrive()
{
	return fs_current_drive;
}

void fs_mount(uint8_t drive)
{
	if (fs_mount_status == MOUNT_STATUS_MOUNTED)
	{
		write("Please dismount the current drive before mounting another drive.");
		return;
	}
	void* info_sector = malloc(512);
	uint32_t magic_number;

	ide_read_sectors(drive, 1, 0, (void*)info_sector);
	
	memory_copy(info_sector, &entry_count, 4);
	memory_copy(info_sector + 4, &magic_number, 4);
	
	free(info_sector);

	fs_mount_status = MOUNT_STATUS_MOUNTED;
	fs_current_drive = drive;

	if (magic_number != MAGIC_NUMBER)
	{
		write("Formating drive...");
		entry_count = 0;
		fs_add_entry((fs_entry) { .name = "ROOT", .ext = "DIR", .type = FS_TYPE_ROOT });
		fs_add_entry((fs_entry) { .name = "CHILD", .ext = "DIR", .type = FS_TYPE_DIR, .parent_dir = 0 });
		fs_add_entry((fs_entry) { .name = "MICHAEL", .ext = "DIR", .type = FS_TYPE_DIR, .parent_dir = 0 });
		fs_add_entry((fs_entry) { .name = "LEAF", .ext = "DIR", .type = FS_TYPE_DIR, .parent_dir = 1 });
		save_fs_data(MAGIC_NUMBER);
	}
	else
	{
		write("Loading ");
		writeInt(entry_count);
		write(" entries...");
		int entries_read = 0;
		for (int s = 0; s < size_in_sectors(entry_count); s++)
		{
			void* buffer = malloc(512);
			ide_read_sectors(drive, 1, s + 1, buffer);
			for (int e = 0; e < 21; e++)
			{
				if (entries_read == entry_count)
				{
					break;
				}
				memory_copy(buffer + (e * 24), &entries[entries_read].name, 8);
				memory_copy(buffer + (e * 24) + 8, &entries[entries_read].ext, 3);
				memory_copy(buffer + (e * 24) + 11, &entries[entries_read].size, 4);
				memory_copy(buffer + (e * 24) + 15, &entries[entries_read].lba, 4);
				memory_copy(buffer + (e * 24) + 19, &entries[entries_read].parent_dir, 4);
				memory_copy(buffer + (e * 24) + 23, &entries[entries_read].type, 1);
				entries[entries_read].children_count = 0;
				entries_read++;
			}
			free(buffer);
		}
		for (int i = 0; i < entry_count; i++)
		{
			if (entries[i].type != FS_TYPE_ROOT)
			{
				entries[entries[i].parent_dir].children[entries[entries[i].parent_dir].children_count++] = i;
			}
		}
	}
}

void fs_eject()
{
	if (fs_mount_status == MOUNT_STATUS_UNMOUNTED)
	{
		writeLine("Cannot eject unmounted filesystem.");
		return;
	}
	save_fs_data(MAGIC_NUMBER);
	fs_mount_status = MOUNT_STATUS_UNMOUNTED;
}

void save_fs_data(uint32_t magic_number)
{
	void* info_sector = malloc(512);
	memory_copy(&entry_count, info_sector, 4);
	memory_copy(&magic_number, info_sector + 4, 4);

	ide_write_sectors(fs_current_drive, 1, 0, info_sector);
	free(info_sector);

	int entries_wrote = 0;

	for (int s = 0; s < size_in_sectors(entry_count); s++)
	{
		void* buffer = malloc(512);
		for (int e = 0; e < 21; e++)
		{
			if (entries_wrote == entry_count)
			{
				break;
			}
			memory_copy(&entries[entries_wrote].name, buffer + (e * 24), 8);
			memory_copy(&entries[entries_wrote].ext, buffer + (e * 24) + 8, 3);
			memory_copy(&entries[entries_wrote].size, buffer + (e * 24) + 11, 4);
			memory_copy(&entries[entries_wrote].lba, buffer + (e * 24) + 15, 4);
			memory_copy(&entries[entries_wrote].parent_dir, buffer + (e * 24) + 19, 4);
			memory_copy(&entries[entries_wrote].type, buffer + (e * 24) + 23, 1);
			entries_wrote++;
		}
		ide_write_sectors(fs_current_drive, 1, s + 1, buffer);
		free(buffer);
	}
}

fs_entry* fs_add_entry(fs_entry entry)
{
	entries[entry_count] = entry;
	if (entry.type != FS_TYPE_ROOT) {
		entries[entry.parent_dir].children[entries[entry.parent_dir].children_count++] = entry_count;
	}
	return &entries[entry_count++];
}

fs_entry* fs_make_dir(char path[], char name[])
{
	fs_entry* parent_entry = fs_search(path);
	if (parent_entry == NULL)
	{
		return NULL; 
	}

	for (int i = 0; i < parent_entry->children_count; i++)
	{
		if (strcmp(name, entries[parent_entry->children[i]].name) == 0)
		{
			return NULL;
		}
	}

	uint32_t parent_index = 0;
	for (int i = 0; i < entry_count; i++)
	{
		if (&entries[i] == parent_entry)
		{
			parent_index = i;
			break;
		}
	}
	fs_entry dir_entry = (fs_entry){ .ext = "DIR", .type = FS_TYPE_DIR, .parent_dir = parent_index};
	strcopy(name, dir_entry.name, 0, strlen(name));
	return fs_add_entry(dir_entry);
}

fs_entry* fs_make_file(char path[], char name[], char ext[], uint32_t size)
{
	fs_entry* parent_entry = fs_search(path);
	return NULL;
}

fs_entry* fs_search(char path[])
{
	fs_entry* current = &entries[0];

	char* tok = strtok(path, '/');
	while (tok != NULL)
	{
		uint8_t error_flag = 1;
		for (int i = 0; i < current->children_count; i++)
		{
			fs_entry* child = &entries[current->children[i]];
			if (strcmp(child->name, tok) == 0)
			{
				current = child;
				error_flag = 0;
				break;
			}
		}
		free(tok);
		if (error_flag)
		{
			return NULL;
		}
		tok = strtok(NULL, '/');
	}
	return current;
}

void fs_print_entry(fs_entry entry)
{
	write(entry.name);
	write(".");
	write(entry.ext);
	write(" - ");
	if (entry.type == FS_TYPE_DIR || entry.type == FS_TYPE_ROOT)
	{
		writeInt(entry.children_count);
		write(" item(s)");
	}
	else if (entry.type == FS_TYPE_FILE)
	{
		writeInt(entry.size);
		write(" byte(s)");
	}
	write("\n");
}

void fs_print_dir_listing(char path[])
{
	fs_entry* dir_entry = fs_search(path);
	if (dir_entry == NULL)
	{
		writeLine("Cannot find the requested directory.");
		return;
	}
	else if (dir_entry->type != FS_TYPE_ROOT && dir_entry->type != FS_TYPE_DIR)
	{
		writeLine("The request entry is not a directory.");
		return;
	}
	write("Directory of \"");
	write(path);
	write("\", ");
	writeInt(dir_entry->children_count);
	write(" item(s)\n");
	for (int i = 0; i < dir_entry->children_count; i++)
	{
		fs_print_entry(entries[dir_entry->children[i]]);
	}
}

void dbg_print_all()
{
	for (int i = 0; i < entry_count; i++)
	{
		fs_print_entry(entries[i]);
	}
}