#include "kernel.h"
#include "io.h"
#include "fs.h"
#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../drivers/disk.h"
#include <stdint.h>

void kernel_main() {
    isr_install();
    irq_install(); 

    clear_screen();
    writeLine("Disk Control Program\nWritten By MICHAEL WANG\n");
    
    ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);

    writeLine("Drives Found:");
    for (int i = 0; i < 4; i++) {
        if (ide_devices[i].reserved == 1) {
            write("    ");
            writeInt(i);
            write(":\\ - ");
            write((char* []) { "ATA", "ATAPI" }[ide_devices[i].type]);
            write(" Drive ");
            writeInt(ide_devices[i].size / 1024 / 2);
            write("MB - ");
            write(ide_devices[i].model);
            write("\n");
        }
    }
    write("\n");

    for (int i = 0; i < 4; i++)
    {
        if (ide_devices[i].reserved == 1 && ide_devices[i].size > 0)
        {
            fs_mount(i);
            write("Mounted filesystem on drive ");
            writeInt(i);
            write("\n\n");
            break;
        }
    }
    if (getMountStatus() == MOUNT_STATUS_UNMOUNTED)
    {
        write("WARNING: No mountable drive detected!\n\n");
    }
    else
    {
        dbg_print_all();
    }

    char input_buffer[256];
    while (1)
    {
        if (getMountStatus() == MOUNT_STATUS_MOUNTED)
        {
            writeInt(getCurrentDrive());
            write(":\\");
        }
        write(">");
        readLine(input_buffer);
        if (strcmp(input_buffer, "exit") == 0)
        {
            write("Ejecting drive...");
            fs_eject();
            writeLine("DONE!");
            return;
        }
        else if (strcmp(input_buffer, "eject") == 0)
        {
            write("Ejecting drive...");
            fs_eject();
            writeLine("DONE!");
        }
        else if (strcmp(input_buffer, "exists") == 0)
        {
            write("PATH>");
            char* path = readAlloc();
            fs_entry* result = fs_search(path);
            if (result == NULL)
            {
                writeLine("Not found.");
            }
            else
            {
                write("Yes, ");
                fs_print_entry(*result);
            }
            free(path);
        }
        else if (strcmp(input_buffer, "dir") == 0)
        {
            write("DIR>");
            char* dir = readAlloc();
            fs_print_dir_listing(dir);
            free(dir);
        }
        else if (strcmp(input_buffer, "mkdir") == 0)
        {
            write("PARENT>");
            char* parent = readAlloc();
            write("NAME>");
            char* name = readAlloc();
            fs_entry* result = fs_make_dir(parent, name);
            if (result == NULL)
            {
                writeLine("Path not found or the directory in the path already exists.");
            }
            else
            {
                fs_print_entry(*result);
            }
            free(parent);
            free(name);
        }
        else if (strcmp(input_buffer, "mkfile") == 0)
        {
            write("PARENT>");
            char* parent = readAlloc();
            write("NAME>");
            char* name = readAlloc();
            write("EXT>");
            char* ext = readAlloc();
            fs_entry* result = fs_make_file(parent, name, ext, 50);
            if (result == NULL)
            {
                writeLine("Path not found or the file in the path already exists.");
            }
            else
            {
                fs_print_entry(*result);
            }
            free(parent);
            free(name);
            free(ext);
        }
    }
}