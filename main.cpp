#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_err.h"
#include "pn532.h"      
#include "pn532_i2c.h"

#define TAG "APP_MAIN"

// ---------- BT remote ----------
static esp_bd_addr_t BT_REMOTE_ADDR = {0xE8,0x07,0xBF,0x19,0x12,0x90};

// ---------- PN532 I2C ----------
#define I2C_PORT        I2C_NUM_0
#define I2C_SDA_GPIO    21
#define I2C_SCL_GPIO    22
#define I2C_FREQ_HZ     400000
#define LED_GPIO        2
#define PN532_ADDR      0x24

// ---------- Motor 28BYJ-48 ----------
#define IN1 25
#define IN2 26
#define IN3 27
#define IN4 14

// ---------- WAV playback ----------
static FILE *wav_file = NULL;
static uint32_t data_bytes_left = 0;
static uint8_t prebuf[2048];
static size_t prebuf_pos = 0, prebuf_len = 0;
static bool a2dp_connected = false;

// ---------- UID global ----------
#define UID_MAX_LEN 7
static uint8_t last_uid[UID_MAX_LEN];
static size_t last_uid_len = 0;

// -------- WAV functions --------
#pragma pack(push,1)
typedef struct {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} wav_header_t;
#pragma pack(pop)

static bool wav_open(const char *path) {
    if (wav_file) fclose(wav_file);
    wav_file = fopen(path,"rb");
    if(!wav_file) return false;

    wav_header_t hdr;
    if(fread(&hdr,sizeof(hdr),1,wav_file)!=1) return false;
    if(strncmp(hdr.riff,"RIFF",4)||strncmp(hdr.wave,"WAVE",4)||
       hdr.audio_format!=1||hdr.num_channels!=2||hdr.sample_rate!=44100||hdr.bits_per_sample!=16)
        return false;

    char chunk_id[4]; uint32_t chunk_size;
    data_bytes_left=0;
    while(fread(chunk_id,1,4,wav_file)==4 && fread(&chunk_size,4,1,wav_file)==1){
        if(!strncmp(chunk_id,"data",4)){data_bytes_left=chunk_size;break;}
        fseek(wav_file,chunk_size,SEEK_CUR);
    }
    prebuf_pos=prebuf_len=0;
    return data_bytes_left>0;
}

static int32_t a2dp_data_cb(uint8_t *data,int32_t len){
    if(!wav_file||data_bytes_left==0) return 0;
    int32_t req=len&~3; if(req<=0) return 0;
    int32_t produced=0;
    while(produced<req && data_bytes_left>0){
        if(prebuf_pos>=prebuf_len){
            size_t to_read=sizeof(prebuf);
            if(to_read>data_bytes_left) to_read=data_bytes_left;
            size_t n=fread(prebuf,1,to_read,wav_file);
            if(n==0) break;
            prebuf_len=n&~0x3;
            prebuf_pos=0;
            data_bytes_left-=(uint32_t)n;
            size_t mis=n-prebuf_len;
            if(mis>0) fseek(wav_file,-(long)mis,SEEK_CUR);
        }
        size_t avail=prebuf_len-prebuf_pos;
        size_t to_copy=req-produced;
        if(to_copy>avail) to_copy=avail;
        memcpy(data+produced,prebuf+prebuf_pos,to_copy);
        prebuf_pos+=to_copy;
        produced+=to_copy;
    }
    return produced;
}

// -------- SD init --------
static void init_sd_spi_cs5(void){
    sdmmc_host_t host=SDSPI_HOST_DEFAULT();
    host.slot=SPI3_HOST;
    spi_bus_config_t bus_cfg={.mosi_io_num=23,.miso_io_num=19,.sclk_io_num=18,.quadwp_io_num=-1,.quadhd_io_num=-1,.max_transfer_sz=64*1024};
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST,&bus_cfg,SPI_DMA_CH_AUTO));
    sdspi_device_config_t slot_config=SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs=GPIO_NUM_5; slot_config.host_id=SPI3_HOST;
    esp_vfs_fat_sdspi_mount_config_t mount_cfg={.format_if_mount_failed=false,.max_files=4};
    sdmmc_card_t *card=NULL;
    ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount("/sdcard",&host,&slot_config,&mount_cfg,&card));
    ESP_LOGI(TAG,"SD montado em /sdcard");
}

// -------- BT init --------
static void a2dp_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param){
    switch(event){
        case ESP_A2D_CONNECTION_STATE_EVT: a2dp_connected=(param->conn_stat.state==ESP_A2D_CONNECTION_STATE_CONNECTED);break;
        case ESP_A2D_AUDIO_STATE_EVT: if(param->audio_stat.state==ESP_A2D_AUDIO_STATE_SUSPEND&&a2dp_connected) esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START); break;
        default: break;
    }
}
static void bt_init(void){
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    esp_bt_controller_config_t bt_cfg=BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    esp_bt_gap_set_device_name("ESP32-A2DP SRC");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,ESP_BT_GENERAL_DISCOVERABLE);
    esp_bt_pin_type_t pin_type=ESP_BT_PIN_TYPE_FIXED; esp_bt_pin_code_t pin_code={'0','0','0','0'};
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(pin_type,4,pin_code));
    ESP_ERROR_CHECK(esp_a2d_register_callback(a2dp_cb));
    ESP_ERROR_CHECK(esp_a2d_source_register_data_callback(a2dp_data_cb));
    ESP_ERROR_CHECK(esp_a2d_source_init());
}

// -------- Motor 28BYJ-48 --------
static void motor_task(void*arg){
    const int seq[8][4]={{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}};
    int step=0; int delay_us=(60*1000000)/(2048*33);
    gpio_pad_select_gpio(IN1);gpio_set_direction(IN1,GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(IN2);gpio_set_direction(IN2,GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(IN3);gpio_set_direction(IN3,GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(IN4);gpio_set_direction(IN4,GPIO_MODE_OUTPUT);
    while(1){
        gpio_set_level(IN1,seq[step][0]);
        gpio_set_level(IN2,seq[step][1]);
        gpio_set_level(IN3,seq[step][2]);
        gpio_set_level(IN4,seq[step][3]);
        step=(step+1)%8;
        ets_delay_us(delay_us);
    }
}

// -------- Main --------
void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    init_sd_spi_cs5();

    // I2C + PN532
    i2c_config_t i2c_cfg={.mode=I2C_MODE_MASTER,.sda_io_num=I2C_SDA_GPIO,.scl_io_num=I2C_SCL_GPIO,.sda_pullup_en=GPIO_PULLUP_ENABLE,.scl_pullup_en=GPIO_PULLUP_ENABLE,.master.clk_speed=I2C_FREQ_HZ};
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT,&i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT,I2C_MODE_MASTER,0,0,0));

    gpio_reset_pin(LED_GPIO);gpio_set_direction(LED_GPIO,GPIO_MODE_OUTPUT);gpio_set_level(LED_GPIO,0);

    // Inicializar PN532 (driver legacy)
    ESP_ERROR_CHECK(pn532_get_firmware());
    ESP_ERROR_CHECK(pn532_sam_config());

    bt_init();

    ESP_LOGI(TAG,"Aguardando NFC para selecionar pasta WAV...");

    while(1){
        esp_err_t er=pn532_read_uid_once();
        if(er==ESP_OK){
            
            memcpy(last_uid,pn532_get_last_uid(),pn532_get_last_uid_len());
            last_uid_len=pn532_get_last_uid_len();

            char path[64]={0};
            
            if(last_uid_len==4){
                if(memcmp(last_uid,(uint8_t[]){0xDE,0xAD,0xBE,0xEF},4)==0) strcpy(path,"/sdcard/1/music1.wav");
                else if(memcmp(last_uid,(uint8_t[]){0xAA,0xBB,0xCC,0xDD},4)==0) strcpy(path,"/sdcard/2/music2.wav");
                else if(memcmp(last_uid,(uint8_t[]){0x11,0x22,0x33,0x44},4)==0) strcpy(path,"/sdcard/3/music3.wav");
                else {ESP_LOGW(TAG,"UID não mapeado"); continue;}
            } else {
                ESP_LOGW(TAG,"UID inesperado"); continue;
            }

            if(!wav_open(path)){ESP_LOGE(TAG,"Falha abrir WAV");continue;}

            
            xTaskCreatePinnedToCore(motor_task,"motor",2048,NULL,5,NULL,APP_CPU_NUM);

            
            ESP_ERROR_CHECK(esp_a2d_source_connect(BT_REMOTE_ADDR));

            while(data_bytes_left>0) vTaskDelay(pdMS_TO_TICKS(200));
            ESP_LOGI(TAG,"Fim audio");
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

