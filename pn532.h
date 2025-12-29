// pn532.h
#pragma once

#include "driver/i2c.h"
#include <stdbool.h>

#define PN532_I2C_ADDRESS 0x24     // O teu módulo usa 0x48
#define PN532_PREAMBLE 0x00
#define PN532_STARTCODE1 0x00
#define PN532_STARTCODE2 0xFF
#define PN532_POSTAMBLE 0x00

#define PN532_HOSTTOPN532 0xD4
#define PN532_PN532TOHOST 0xD5

#define PN532_COMMAND_GETFIRMWAREVERSION 0x02
#define PN532_COMMAND_SAMCONFIGURATION 0x14
#define PN532_COMMAND_INLISTPASSIVETARGET 0x4A

typedef struct {
    i2c_port_t port;
} pn532_t;

void pn532_init(pn532_t *dev, i2c_port_t port);
uint32_t pn532_get_firmware_version(pn532_t *dev);
bool pn532_SAMConfiguration(pn532_t *dev);
bool pn532_ReadPassiveTargetID(pn532_t *dev, uint8_t cardbaud, uint8_t *uid, uint8_t *uidLength);
