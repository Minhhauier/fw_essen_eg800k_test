#include <stdio.h>
#include <esp_log.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

#include "mqtt.h"
#include "config_parameter.h" 
#include "at_command.h"
#include "encrypt_decrypt.h"
#include "system_manage.h"
#include "gps_eg800k.h"
#include"control_led.h"
//#include "control_relay.h"

static char data[BUF_SIZE_SIM];

#define MQTT_TAG        "MQTT_DATA"
#define DATA_SIM_TAG    "SIM_DATA"

static char topic[256];


void sim_mqtt_task(void *pvParameters){
    TickType_t gps_tick = xTaskGetTickCount();
    snprintf(topic,256,"%s/SmartEVsafe",PUB);
    while (mqtt_sub_success!=true){
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    //publish_version(HW_VERSION,FW_VERSION,0);
    bool mqtt_connect=true;
    char *recv_msg;
    while (1)
    {
        if(mqtt_sub_success){
            if(mqtt_connect){
                mqtt_connect=false;
                set_group_led(&charge_led,COLOR_BLUE,5);
            }
            if(xQueueReceive(mqtt_queue_handle,data,pdMS_TO_TICKS(50))==pdTRUE){
                //ESP_LOGI(MQTT_TAG,"%s",data);
                convert_to_json(data);
            }
            if(xQueueReceive(sim_at_queue_handle,data,pdMS_TO_TICKS(50))==pdTRUE){
                //ESP_LOGI(DATA_SIM_TAG,"%s",data);
                if(strstr(data,"+QMTSTAT: 1,1") || strstr(data,"RDY")||strstr(data,"+QMTSTAT: 1,2")){
                    mqtt_sub_success=false;
                }
                else if(strstr(data,"AT+QMTPUBEX") && strstr(data,"ERROR")){
                    mqtt_sub_success=false;
                }
            }
            if(xQueueReceive(gps_queue_handle,data,pdMS_TO_TICKS(50))==pdTRUE)
            {
                //ESP_LOGI("GPS data","%s",data);
                if(strstr(data,"+QGPSLOC:")!=NULL) gps_process_data(data);
            }
            if(xQueueReceive(publish_queue_handle,data,pdMS_TO_TICKS(50))==pdTRUE){
               // ESP_LOGI(MQTT_TAG,"%s",recv_msg);
                if(strstr(data,"serial_number")!=NULL) {
                    mqtt_pub(topic,data); 
        
                } 
            }
            //Gps function - send read gps request each 28 minutes 
            // if(xTaskGetTickCount() - gps_tick >= pdMS_TO_TICKS(1700000)){
            //     gps_tick = xTaskGetTickCount();  
            //     send_at("AT+QGPSLOC=0");
            // }
        }
        else{
            if(mqtt_connect==false){
                mqtt_connect=true;
                set_group_led(&charge_led,COLOR_RED,5);
            }
            mqtt_init();
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    
}
