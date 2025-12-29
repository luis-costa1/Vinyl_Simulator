// pn532.c
#include "pn532.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "PN532";

static esp_err_t pn532_write(pn532_t *dev, uint8_t *data, size_t len)
{
    return i2c_master_write_to_device(dev->port, PN532_I2C_ADDRESS, data, len, 100 / portTICK_PERIOD_MS);
}

static esp_err_t pn532_read(pn532_t *dev, uint8_t *data, size_t len)
{
    return i2c_master_read_from_device(dev->port, PN532_I2C_ADDRESS, data, len, 100 / portTICK_PERIOD_MS);
}

void pn532_init(pn532_t *dev, i2c_port_t port)
{
    dev->port = port;
    vTaskDelay(pdMS_TO_TICKS(100));
}

uint32_t pn532_get_firmware_version(pn532_t *dev)
{
    uint8_t cmd[] = {PN532_PREAMBLE, PN532_STARTCODE1, PN532_STARTCODE2, 0x02,
                     0xFE, PN532_HOSTTOPN532, PN532_COMMAND_GETFIRMWAREVERSION,
                     (uint8_t)(0xFF - (PN532_HOSTTOPN532 + PN532_COMMAND_GETFIRMWAREVERSION) + 1),
                     PN532_POSTAMBLE};

    pn532_write(dev, cmd, sizeof(cmd));
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t resp[24];
    if (pn532_read(dev, resp, sizeof(resp)) != ESP_OK) return 0;

    return (resp[14] << 24) | (resp[15] << 16) | (resp[16] << 8) | resp[17];
}

bool pn532_SAMConfiguration(pn532_t *dev)
{
    uint8_t cmd[] = {
        PN532_PREAMBLE, PN532_STARTCODE1, PN532_STARTCODE2,
        0x05, 0xFB,
        PN532_HOSTTOPN532, PN532_COMMAND_SAMCONFIGURATION,
        0x01, 0x14, 0x01,
        (uint8_t)(0xFF - (PN532_HOSTTOPN532 + PN532_COMMAND_SAMCONFIGURATION + 0x01 + 0x14 + 0x01) + 1),
        PN532_POSTAMBLE
    };

    pn532_write(dev, cmd, sizeof(cmd));
    vTaskDelay(pdMS_TO_TICKS(50));
    return true;
}

bool pn532_ReadPassiveTargetID(pn532_t *dev, uint8_t baud, uint8_t *uid, uint8_t *uidLength)
{
    uint8_t cmd[] = {
        PN532_PREAMBLE, PN532_STARTCODE1, PN532_STARTCODE2,
        0x03, 0xFD,
        PN532_HOSTTOPN532, PN532_COMMAND_INLISTPASSIVETARGET, 0x01,
        (uint8_t)(0xFF - (PN532_HOSTTOPN532 + PN532_COMMAND_INLISTPASSIVETARGET + 0x01) + 1),
        PN532_POSTAMBLE
    };

    pn532_write(dev, cmd, sizeof(cmd));
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t resp[32];
    if (pn532_read(dev, resp, sizeof(resp)) != ESP_OK) return false;

    if (resp[12] != 1) return false;

    *uidLength = resp[17];
    memcpy(uid, &resp[18], *uidLength);
    return true;
}
