#include <stdint.h>

#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01

#define IDE_ATA        0x00
#define IDE_ATAPI      0x01

struct ide_device {
    uint8_t  reserved;    // 0 (Empty) or 1 (This Drive really exists).
    uint8_t  channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
    uint8_t  drive;       // 0 (Master Drive) or 1 (Slave Drive).
    uint16_t type;        // 0: ATA, 1:ATAPI.
    uint16_t signature;   // Drive Signature
    uint16_t capabilities;// Features.
    uint32_t   commandSets; // Command Sets Supported.
    uint32_t   size;        // Size in Sectors.
    uint8_t  model[41];   // Model in string.
} ide_devices[4];

void ide_initialize(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4);
void ide_atapi_eject(uint8_t drive);
void ide_read_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void* buffer);
void ide_write_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void* buffer);



#endif // DISK_H
