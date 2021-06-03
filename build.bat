nasm boot/bootsect.asm -f bin -o bin/bootsect.bin
nasm boot/kernel_entry.asm -f elf -o bin/obj/kernel_entry.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c cpu/idt.c -o bin/obj/idt.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c cpu/isr.c -o bin/obj/isr.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c cpu/ports.c -o bin/obj/ports.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c cpu/timer.c -o bin/obj/timer.o
nasm cpu/interrupt.asm -f elf -o bin/obj/interrupt.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c drivers/keyboard.c -o bin/obj/keyboard.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c drivers/screen.c -o bin/obj/screen.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c drivers/disk.c -o bin/obj/disk.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c kernel/kernel.c -o bin/obj/kernel.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c kernel/io.c -o bin/obj/io.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c kernel/fs.c -o bin/obj/fs.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c libc/mem.c -o bin/obj/mem.o
"i686/bin/i686-elf-gcc.exe" -ffreestanding -c libc/string.c -o bin/obj/string.o
"i686/bin/i686-elf-ld.exe" -o bin/kernel.bin -Ttext 0x1000 bin/obj/kernel_entry.o bin/obj/idt.o bin/obj/isr.o bin/obj/ports.o bin/obj/timer.o bin/obj/interrupt.o bin/obj/screen.o bin/obj/keyboard.o bin/obj/disk.o bin/obj/kernel.o bin/obj/io.o bin/obj/fs.o bin/obj/mem.o bin/obj/string.o --oformat binary
cat bin/bootsect.bin bin/kernel.bin > bin/os-image.img
pause