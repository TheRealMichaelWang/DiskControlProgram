#include "disk.h"
#include "../cpu/ports.h"
#include "../cpu/timer.h"
#include "../kernel/io.h"
#include <stdint.h>

#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

#define      ATAPI_CMD_READ       0xA8
#define      ATAPI_CMD_EJECT      0x1B

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

// Directions:
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01

struct IDEChannelRegisters {
    uint16_t base;  // I/O Base.
    uint16_t ctrl;  // Control Base
    uint16_t bmide; // Bus Master IDE
    uint8_t  nIEN;  // nIEN (No Interrupt);
} channels[2];

uint8_t ide_buf[2048] = { 0 };
static uint8_t ide_irq_invoked = 0;
static uint8_t atapi_packet[12] = { 0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t package[1] = { 0 };

uint8_t ide_read(uint8_t channel, uint8_t reg);
void ide_write(uint8_t channel, uint8_t reg, uint8_t data);
void ide_read_buffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads);

static inline void insl(uint32_t port, void* addr, int cnt) {
    asm volatile (
        "cld;"
        "repne; insl;"
        : "=D" (addr), "=c" (cnt)
        : "d" (port), "0" (addr), "1" (cnt)
        : "memory", "cc");
}

void ide_initialize(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4) {
    int i, j, k, count = 0;

    // 1- Detect I/O Ports which interface IDE Controller:
    channels[ATA_PRIMARY].base = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
    channels[ATA_PRIMARY].ctrl = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
    channels[ATA_SECONDARY].base = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
    channels[ATA_SECONDARY].ctrl = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
    channels[ATA_PRIMARY].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

    // 2- Disable IRQs:
    ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    // 3- Detect ATA-ATAPI Devices:
    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++) {

            uint8_t err = 0, type = IDE_ATA, status;
            ide_devices[count].reserved = 0; // Assuming that no drive here.

            // (I) Select Drive:
            ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
            sleep(1); // Wait 1ms for drive select to work.

            // (II) Send ATA Identify Command:
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            sleep(1); // This function should be implemented in your OS. which waits for 1 ms.
                      // it is based on System Timer Device Driver.

            // (III) Polling:
            if (ide_read(i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.

            while (1) {
                status = ide_read(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) { err = 1; break; } // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
            }

            // (IV) Probe for ATAPI Devices:

            if (err != 0) {
                uint8_t cl = ide_read(i, ATA_REG_LBA1);
                uint8_t ch = ide_read(i, ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (may not be a device).

                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                sleep(1);
            }

            // (V) Read Identification Space of the Device:
            ide_read_buffer(i, ATA_REG_DATA, (uint32_t)ide_buf, 128);

            // (VI) Read Device Parameters:
            ide_devices[count].reserved = 1;
            ide_devices[count].type = type;
            ide_devices[count].channel = i;
            ide_devices[count].drive = j;
            ide_devices[count].signature = *((uint16_t*)(ide_buf + ATA_IDENT_DEVICETYPE));
            ide_devices[count].capabilities = *((uint16_t*)(ide_buf + ATA_IDENT_CAPABILITIES));
            ide_devices[count].commandSets = *((uint32_t*)(ide_buf + ATA_IDENT_COMMANDSETS));

            // (VII) Get Size:
            if (ide_devices[count].commandSets & (1 << 26))
                // Device uses 48-Bit Addressing:
                ide_devices[count].size = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                ide_devices[count].size = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA));

            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for (k = 0; k < 40; k += 2) {
                ide_devices[count].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                ide_devices[count].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
            }
            ide_devices[count].model[40] = '\0'; // Terminate String.

            count++;
        }
}

uint8_t ide_read(uint8_t channel, uint8_t reg) {
    uint8_t result;
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        result = port_byte_in(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = port_byte_in(channels[channel].base + reg - 0x06);
    else if (reg < 0x0E)
        result = port_byte_in(channels[channel].ctrl + reg - 0x0A);
    else if (reg < 0x16)
        result = port_byte_in(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    return result;
}

void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        port_byte_out(channels[channel].base + reg - 0x00, data);
    else if (reg < 0x0C)
        port_byte_out(channels[channel].base + reg - 0x06, data);
    else if (reg < 0x0E)
        port_byte_out(channels[channel].ctrl + reg - 0x0A, data);
    else if (reg < 0x16)
        port_byte_out(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

void ide_read_buffer(uint8_t channel, uint8_t reg, uint32_t buffer,
    uint32_t quads) {
    /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
     *           ESP for all of the code the compiler generates between the inline
     *           assembly blocks.
     */
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    asm("pushw %es; pushw %ax; movw %ds, %ax; movw %ax, %es; popw %ax;");
    if (reg < 0x08)
        insl(channels[channel].base + reg - 0x00, (void*)buffer, quads);
    else if (reg < 0x0C)
        insl(channels[channel].base + reg - 0x06, (void*)buffer, quads);
    else if (reg < 0x0E)
        insl(channels[channel].ctrl + reg - 0x0A, (void*)buffer, quads);
    else if (reg < 0x16)
        insl(channels[channel].bmide + reg - 0x0E, (void*)buffer, quads);
    asm("popw %es;");
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

uint8_t ide_polling(uint8_t channel, uint32_t advanced_check) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for (int i = 0; i < 4; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.

     // (II) Wait for BSY to be cleared:
     // -------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait for BSY to be zero.

    if (advanced_check) {
        uint8_t state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

        // (III) Check For Errors:
        // -------------------------------------------------
        if (state & ATA_SR_ERR)
            return 2; // Error.

         // (IV) Check If Device fault:
         // -------------------------------------------------
        if (state & ATA_SR_DF)
            return 1; // Device Fault.

         // (V) Check DRQ:
         // -------------------------------------------------
         // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
            return 3; // DRQ should be set

    }

    return 0; // No Error.
}

uint8_t ide_print_error(uint32_t drive, uint8_t err) {
    if (err == 0)
        return err;

    write("IDE:");
    if (err == 1) { write("- Device Fault\n     "); err = 19; }
    else if (err == 2) {
        unsigned char st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF) { write("- No Address Mark Found\n     ");   err = 7; }
        if (st & ATA_ER_TK0NF) { write("- No Media or Media Error\n     ");   err = 3; }
        if (st & ATA_ER_ABRT) { write("- Command Aborted\n     ");      err = 20; }
        if (st & ATA_ER_MCR) { write("- No Media or Media Error\n     ");   err = 3; }
        if (st & ATA_ER_IDNF) { write("- ID mark not Found\n     ");      err = 21; }
        if (st & ATA_ER_MC) { write("- No Media or Media Error\n     ");   err = 3; }
        if (st & ATA_ER_UNC) { write("- Uncorrectable Data Error\n     ");   err = 22; }
        if (st & ATA_ER_BBK) { write("- Bad Sectors\n     ");       err = 13; }
    }
    else  if (err == 3) { write("- Reads Nothing\n     "); err = 23; }
    else  if (err == 4) { write("- Write Protected\n     "); err = 8; }
    write("- [");
    write((char* []) { "Primary", "Secondary" } [ide_devices[drive].channel] );
    write(" ");
    write((char* []) { "Master", "Slave" } [ide_devices[drive].drive] );
    write("]: ");
    write(ide_devices[drive].model);
    write("\n");

    return err;
}

/* ATA/ATAPI Read/Write Modes:
    * ++++++++++++++++++++++++++++++++
    *  Addressing Modes:
    *  ================
    *   - LBA28 Mode.     (+)
    *   - LBA48 Mode.     (+)
    *   - CHS.            (+)
    *  Reading Modes:
    *  ================
    *   - PIO Modes (0 : 6)       (+) // Slower than DMA, but not a problem.
    *   - Single Word DMA Modes (0, 1, 2).
    *   - Double Word DMA Modes (0, 1, 2).
    *   - Ultra DMA Modes (0 : 6).
    *  Polling Modes:
    *  ================
    *   - IRQs
    *   - Polling Status   (+) // Suitable for Singletasking
    */

uint8_t ide_ata_access(uint8_t direction, uint8_t drive, uint32_t lba, uint8_t numsects, void* read_buffer) {
    uint8_t lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
    uint8_t lba_io[6];
    uint32_t  channel = ide_devices[drive].channel; // Read the Channel.
    uint32_t  slavebit = ide_devices[drive].drive; // Read the Drive [Master/Slave]
    uint32_t  bus = channels[channel].base; // Bus Base, like 0x1F0 which is also data port.
    uint32_t  words = 256; // Almost every ATA drive has a sector-size of 512-byte.
    uint16_t cyl, i;
    uint8_t head, sect, err;

    // (I) Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) { // Sure Drive should support LBA in this case, or you are
                             // giving a wrong LBA.
       // LBA48:
        lba_mode = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        head = 0; // Lower 4-bits of HDDEVSEL are not used here.
    }
    else if (ide_devices[drive].capabilities & 0x200) { // Drive supports LBA?
    // LBA28:
        lba_mode = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head = (lba & 0xF000000) >> 24;
    }
    else {
        // CHS:
        lba_mode = 0;
        sect = (lba % 63) + 1;
        cyl = (lba + 1 - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba + 1 - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
    }

    // (II) See if drive supports DMA or not;
    dma = 0; // We don't support DMA

    // (III) Wait if the drive is busy;
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait if busy.

    // (IV) Select Drive from the controller;
    if (lba_mode == 0)
        ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA


    // (V) Write Parameters;
    if (lba_mode == 2) {
        ide_write(channel, ATA_REG_SECCOUNT1, 0);
        ide_write(channel, ATA_REG_LBA3, lba_io[3]);
        ide_write(channel, ATA_REG_LBA4, lba_io[4]);
        ide_write(channel, ATA_REG_LBA5, lba_io[5]);
    }
    ide_write(channel, ATA_REG_SECCOUNT0, numsects);
    ide_write(channel, ATA_REG_LBA0, lba_io[0]);
    ide_write(channel, ATA_REG_LBA1, lba_io[1]);
    ide_write(channel, ATA_REG_LBA2, lba_io[2]);

    // (VI) Select the command and send it;
    // Routine that is followed:
    // If ( DMA & LBA48)   DO_DMA_EXT;
    // If ( DMA & LBA28)   DO_DMA_LBA;
    // If ( DMA & LBA28)   DO_DMA_CHS;
    // If (!DMA & LBA48)   DO_PIO_EXT;
    // If (!DMA & LBA28)   DO_PIO_LBA;
    // If (!DMA & !LBA#)   DO_PIO_CHS;

    if (lba_mode == 0 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 2 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 1 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 2 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA_EXT;
    if (lba_mode == 0 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 1 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 2 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 1 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 2 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA_EXT;
    ide_write(channel, ATA_REG_COMMAND, cmd);               // Send the Command.

    if (dma)
        if (direction == 0);
    // DMA Read.
        else;
    // DMA Write.
    else
        if (direction == 0) {
            // PIO Read.
            uint16_t* buffer = (uint16_t*)read_buffer;
            for (i = 0; i < numsects; i++) {
                if (err = ide_polling(channel, 1))
                    return err; // Polling, set error and exit if there is.
                //asm("pushw %es");
                //asm("mov %%ax, %%es" : : "a"(selector));
                //asm("rep insw" : : "c"(words), "d"(bus), "D"(edi)); // Receive Data.
                //asm("popw %es");
                //edi += (words * 2);
                for (int j = 0; j < 256; j++)
                {
                    buffer[j] = port_word_in(0x1F0);
                }
                buffer = (uint16_t*)((uint32_t)buffer + 256);
            }
        }
        else {
            // PIO Write.
            uint16_t* buffer = (uint16_t*)read_buffer;
            for (i = 0; i < numsects; i++) {
                ide_polling(channel, 0); // Polling.
                //asm("pushw %ds");
                //asm("mov %%ax, %%ds"::"a"(selector));
                //asm("rep outsw"::"c"(words), "d"(bus), "S"(edi)); // Send Data
                //asm("popw %ds");
                //edi += (words * 2);
                for (int j = 0; j < 256; j++)
                {
                    port_word_out(0x1F0, buffer[j]);
                }
                buffer = (uint16_t*)((uint32_t)buffer + 256);
            }
            ide_write(channel, ATA_REG_COMMAND, (char[]) {
                ATA_CMD_CACHE_FLUSH,
                    ATA_CMD_CACHE_FLUSH,
                    ATA_CMD_CACHE_FLUSH_EXT
            }[lba_mode]);
            ide_polling(channel, 0); // Polling.
        }

    return 0; // Easy, isn't it?
}

void ide_wait_irq() {
    while (!ide_irq_invoked)
        ;
    ide_irq_invoked = 0;
}

void ide_irq() {
    ide_irq_invoked = 1;
}

uint8_t ide_atapi_read(uint8_t drive, uint32_t lba, uint8_t numsects, void* read_buffer) {
    uint32_t   channel = ide_devices[drive].channel;
    uint32_t   slavebit = ide_devices[drive].drive;
    uint32_t   bus = channels[channel].base;
    uint32_t   words = 1024; // Sector Size. ATAPI drives have a sector size of 2048 bytes.
    uint8_t  err;
    int i;

    // Enable IRQs:
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);

    // (I): Setup SCSI Packet:
    // ------------------------------------------------------------------
    atapi_packet[0] = ATAPI_CMD_READ;
    atapi_packet[1] = 0x0;
    atapi_packet[2] = (lba >> 24) & 0xFF;
    atapi_packet[3] = (lba >> 16) & 0xFF;
    atapi_packet[4] = (lba >> 8) & 0xFF;
    atapi_packet[5] = (lba >> 0) & 0xFF;
    atapi_packet[6] = 0x0;
    atapi_packet[7] = 0x0;
    atapi_packet[8] = 0x0;
    atapi_packet[9] = numsects;
    atapi_packet[10] = 0x0;
    atapi_packet[11] = 0x0;

    // (II): Select the drive:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_HDDEVSEL, slavebit << 4);

    // (III): Delay 400 nanoseconds for select to complete:
    // ------------------------------------------------------------------
    for (int i = 0; i < 4; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns.

    // (IV): Inform the Controller that we use PIO mode:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_FEATURES, 0);         // PIO mode.

    // (V): Tell the Controller the size of buffer:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_LBA1, (words * 2) & 0xFF);   // Lower Byte of Sector Size.
    ide_write(channel, ATA_REG_LBA2, (words * 2) >> 8);   // Upper Byte of Sector Size.

    // (VI): Send the Packet Command:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);      // Send the Command.

    // (VII): Waiting for the driver to finish or return an error code:
    // ------------------------------------------------------------------
    if (err = ide_polling(channel, 1)) return err;         // Polling and return if error.

    // (VIII): Sending the packet data:
    // ------------------------------------------------------------------
    asm("rep   outsw" : : "c"(6), "d"(bus), "S"(atapi_packet));   // Send Packet Data

    // (IX): Receiving Data:
    // ------------------------------------------------------------------
    uint16_t* buffer = (uint16_t*)read_buffer;
    for (i = 0; i < numsects; i++) {
        ide_wait_irq();                  // Wait for an IRQ.
        if (err = ide_polling(channel, 1))
            return err;      // Polling and return if error.
        for (int j = 0; j < 256; j++)
        {
            buffer[j] = port_word_in(0x1F0);
        }
        buffer = (uint16_t*)((uint32_t)buffer + 256);
    }

    // (X): Waiting for an IRQ:
    // ------------------------------------------------------------------
    ide_wait_irq();

    // (XI): Waiting for BSY & DRQ to clear:
    // ------------------------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ))
        ;

    return 0; // Easy, ... Isn't it?
}

void ide_read_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void *buffer) {

    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].reserved == 0) package[0] = 0x1;      // Drive Not Found!

    // 2: Check if inputs are valid:
    // ==================================
    else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
        package[0] = 0x2;                     // Seeking to invalid position.

     // 3: Read in PIO Mode through Polling & IRQs:
     // ============================================
    else {
        uint8_t err;
        if (ide_devices[drive].type == IDE_ATA)
            err = ide_ata_access(ATA_READ, drive, lba, numsects, buffer);
        else if (ide_devices[drive].type == IDE_ATAPI)
            for (int i = 0; i < numsects; i++) {
                void* readbuffer = buffer;
                err = ide_atapi_read(drive, lba + i, 1, readbuffer);
                readbuffer = (void*)((uint32_t)readbuffer + 512);
            }
        package[0] = ide_print_error(drive, err);
    }
}
// package[0] is an entry of an array. It contains the Error Code.

void ide_write_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void* buffer) {

    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].reserved == 0)
        package[0] = 0x1;      // Drive Not Found!
     // 2: Check if inputs are valid:
     // ==================================
    else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
        package[0] = 0x2;                     // Seeking to invalid position.
     // 3: Read in PIO Mode through Polling & IRQs:
     // ============================================
    else {
        uint8_t err;
        if (ide_devices[drive].type == IDE_ATA)
            err = ide_ata_access(ATA_WRITE, drive, lba, numsects, buffer);
        else if (ide_devices[drive].type == IDE_ATAPI)
            err = 4; // Write-Protected.
        package[0] = ide_print_error(drive, err);
    }
}

void ide_atapi_eject(uint8_t drive) {
    uint32_t   channel = ide_devices[drive].channel;
    uint32_t   slavebit = ide_devices[drive].drive;
    uint32_t   bus = channels[channel].base;
    uint32_t   words = 2048 / 2;               // Sector Size in Words.
    uint8_t  err = 0;
    ide_irq_invoked = 0;

    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].reserved == 0)
        package[0] = 0x1;      // Drive Not Found!
     // 2: Check if drive isn't ATAPI:
     // ==================================
    else if (ide_devices[drive].type == IDE_ATA)
        package[0] = 20;         // Command Aborted.
     // 3: Eject ATAPI Driver:
     // ============================================
    else {
        // Enable IRQs:
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);

        // (I): Setup SCSI Packet:
        // ------------------------------------------------------------------
        atapi_packet[0] = ATAPI_CMD_EJECT;
        atapi_packet[1] = 0x00;
        atapi_packet[2] = 0x00;
        atapi_packet[3] = 0x00;
        atapi_packet[4] = 0x02;
        atapi_packet[5] = 0x00;
        atapi_packet[6] = 0x00;
        atapi_packet[7] = 0x00;
        atapi_packet[8] = 0x00;
        atapi_packet[9] = 0x00;
        atapi_packet[10] = 0x00;
        atapi_packet[11] = 0x00;

        // (II): Select the Drive:
        // ------------------------------------------------------------------
        ide_write(channel, ATA_REG_HDDEVSEL, slavebit << 4);

        // (III): Delay 400 nanosecond for select to complete:
        // ------------------------------------------------------------------
        for (int i = 0; i < 4; i++)
            ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.

         // (IV): Send the Packet Command:
         // ------------------------------------------------------------------
        ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);      // Send the Command.

        // (V): Waiting for the driver to finish or invoke an error:
        // ------------------------------------------------------------------
        err = ide_polling(channel, 1);            // Polling and stop if error.

        // (VI): Sending the packet data:
        // ------------------------------------------------------------------
      asm("rep   outsw"::"c"(6), "d"(bus), "S"(atapi_packet));// Send Packet Data
      ide_wait_irq();                  // Wait for an IRQ.
      err = ide_polling(channel, 1);            // Polling and get error code.
      if (err == 3) err = 0; // DRQ is not needed here
      package[0] = ide_print_error(drive, err); // Return;
    }
}