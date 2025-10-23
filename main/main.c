#include <stdio.h>
#include <esp_log.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

#include "system_manage.h"
#include "config_parameter.h"
#include "mqtt.h"
#include "uart.h"
#include "encrypt_decrypt.h"
#include "at_command.h"
#include "system_manage.h"
#include "pzem.h"
#include "control_led.h"
#include "gps_eg800k.h"
#include "ota.h"
#include "config_gpio.h"

#define TAG_ROOT "ROOT"

char device_name[25];
static char m[BUF_SIZE_SIM];
TaskHandle_t sim_task = NULL;
TaskHandle_t pzem_ds18b20_task = NULL;
TaskHandle_t read_sim_uart_task = NULL;
//static char data[BUF_SIZE_SIM];


void track_ram(){
    float power[6]={50,50,50,50,50,50};
    int volt[6]={5,5,5,5,5,5};
    while (true)
    {
        //publish_data_1gun_only(1,0,0);
        //publish_data_power(power,volt);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        printf("Free internal RAM: %d bytes\n", free_internal);

        size_t free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        printf("Free 8bit accessible RAM: %d bytes\n", free_8bit);

        size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
        printf("Free DMA-capable RAM: %d bytes\n", free_dma);
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
    
}
void get_device_name(char *device_name) {
    uint8_t mac[6];
    esp_err_t res = esp_efuse_mac_get_default(mac);
    if (res == ESP_OK) {
        snprintf(device_name, 25, "EV%02x%02x%02x%02x%02x",
                 mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGE(TAG_ROOT, "Failed to read MAC address");
    }
}
void app_main(void)
{    
    get_device_name(device_name);
    ESP_LOGI(TAG_ROOT,"==Device name: %s==",device_name);
    uart_sim_init();

    //convert_to_json(mqtt_data);
    //configure_uart_dynamic_Pzem(UART_PZEM_NUM,9600,TX_PZEM,RX_PZEM);
    all_led_by_status(0);
    set_group_led(&charge_led,COLOR_RED,5);
    mqtt_init();
    //gps_init();
    convert_to_json(NULL);

    config_gpio_detect_zero();

   if(read_sim_uart_task==NULL) xTaskCreate(read_and_send_to_queue_task,"read_sim_uart_task",1024*4,NULL,10,&read_sim_uart_task);
   if(sim_task==NULL) xTaskCreate(sim_mqtt_task,"sim_mqtt_task",1024*8,NULL,10,&sim_task);
   if(pzem_ds18b20_task==NULL) xTaskCreate(pzem_task,"pzem task",1024*4,NULL,10,&pzem_ds18b20_task); 

    //xTaskCreate(gps_get_position_task,"test gps",1024*4,NULL,10,NULL);
    //xTaskCreate(track_ram,"ram task",1024*8,NULL,10,NULL);
    //xTaskCreate(test_ota,"test ota",1024*8,NULL,10,NULL);
   // xTaskCreate(detect_zero,"detect zero",1024*8,NULL,10,NULL);
    while (1)
    {
        // int len = uart_read_bytes(UART_SIM_NUM,m,BUF_SIZE_SIM,20/portTICK_PERIOD_MS);
        // if(len>0) printf("%s",m);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
