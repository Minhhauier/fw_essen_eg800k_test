#include <stdio.h>
#include <stdlib.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "uart.h"
#include "config_parameter.h"
#include "at_command.h"
#include "encrypt_decrypt.h"
#include "mqtt.h"
// #include "control_relay.h"
bool send_posible = false;
QueueHandle_t sim_at_queue_handle;
QueueHandle_t mqtt_queue_handle;
QueueHandle_t gps_queue_handle;
QueueHandle_t publish_queue_handle;

static char data[BUF_SIZE_SIM];
void send_at_get_respond(char *cmd, int timeout)
{
    ESP_LOGI("SIM", "sent: %s", cmd);
    uart_write_bytes(UART_SIM_NUM, cmd, strlen(cmd));
    uart_write_bytes(UART_SIM_NUM, "\r\n", 2);
    int total_len = 0;
    int count = 0;
    for (int i = 0; i < timeout; i += 100)
    {
        int len = uart_read_bytes(UART_SIM_NUM, data + total_len, BUF_SIZE_SIM - total_len - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            total_len = total_len + len;
            count = 0;
        }
        else
            count++;
        if (count > 3)
            break;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (total_len > 0)
    {
        data[total_len] = '\0';
        printf("RX_sent_at: %s\r\n", data);
    }
}

void send_at(char *cmd)
{
    //ESP_LOGI("SIM","sent: %s",cmd);
    vTaskDelay(30/portTICK_PERIOD_MS);
    uart_write_bytes(UART_SIM_NUM, cmd, strlen(cmd));
    uart_write_bytes(UART_SIM_NUM, "\r\n", 2);
}
char *get_respond(int timeout)
{
    int total_len = 0;
    int count = 0;
    for (int i = 0; i < timeout; i += 100)
    {
        int len = uart_read_bytes(UART_SIM_NUM, data + total_len, BUF_SIZE_SIM - total_len - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            total_len = total_len + len;
            count = 0;
        }
        else
            count++;
        if (count > 3)
            break;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (total_len > 0)
    {
        data[total_len] = '\0';
        printf("RX: %s\r\n", data);
        char *data_new = calloc(total_len + 1, sizeof(char));
        strcpy(data_new, data);
        return data_new;
    }
    return NULL;
}

// void read_and_send_to_queue_task(void *pvParameters)
// {
//     sim_at_queue_handle = xQueueCreate(10,BUF_SIZE_SIM); // 10: có thể chứa tối đa 10 phần tử, BUF_SIZE_SIM: kích thước của mỗi phần tử
//     mqtt_queue_handle = xQueueCreate(10,BUF_SIZE_SIM);
//     gps_queue_handle = xQueueCreate(10,BUF_SIZE_SIM);
//     publish_queue_handle = xQueueCreate(10,sizeof(char *));
//    // is_relay_init();
//     int timeout=1000;
//     while (1)
//     {
//         if (read_enable)
//         {
//             int total_len=0;int count=0;
//             for(int i=0;i<timeout;i+=20){
//                 int len = uart_read_bytes(UART_SIM_NUM,data+total_len,BUF_SIZE_SIM-total_len-1,30/portTICK_PERIOD_MS);
//                 if(len>0){
//                     total_len=total_len+len;
//                     count=0;
//                 }
//                 if(strstr(data,"\r\n")) break;
//             else count++;
//             if(count>3) break;
//             vTaskDelay(20/portTICK_PERIOD_MS);
//             }
//             if(total_len>0){
//                 data[total_len] = '\0';
//                 //printf("Queue RX: %s",data);
//                 if(strstr(data,"+QMTRECV:")!=NULL) xQueueSend(mqtt_queue_handle,data,portMAX_DELAY);
//                 else if(strstr(data,"+QGPSLOC:")!=NULL) xQueueSend(gps_queue_handle,data,portMAX_DELAY);
//                 else xQueueSend(sim_at_queue_handle,data,portMAX_DELAY);
//                 if (strchr(data,'>')){
//                     send_posible=true;
//                     printf("detected >\r\n");
//                 }
//                 else if(strstr(data,"AT+QMTPUBEX=1,0,0,0")){
//                     vTaskDelay(100/portTICK_PERIOD_MS);
//                     send_posible=true;
//                 }
//             }
//         }
//         vTaskDelay(100/portTICK_PERIOD_MS);
//     }
//    // parse_js
// }
void read_and_send_to_queue_task(void *pvParameters){
    sim_at_queue_handle = xQueueCreate(10,BUF_SIZE_SIM); // 10: có thể chứa tối đa 10 phần tử, BUF_SIZE_SIM: kích thước của mỗi phần tử
    mqtt_queue_handle = xQueueCreate(10,BUF_SIZE_SIM);
    gps_queue_handle = xQueueCreate(10,BUF_SIZE_SIM);
    publish_queue_handle = xQueueCreate(10,BUF_SIZE_SIM);
   // is_relay_init();
   while (1)
   {
     int len = uart_read_bytes(UART_SIM_NUM,data,BUF_SIZE_SIM,20);
     if(len>0){
        data[len]='\0';
        //printf("uart recieved data\r\n");
        if(strstr(data,"+QMTRECV:")!=NULL) xQueueSend(mqtt_queue_handle,data,portMAX_DELAY);
        else if(strstr(data,"+QGPSLOC:")!=NULL) xQueueSend(gps_queue_handle,data,portMAX_DELAY);
        else if(strstr(data,"\"command_type\":101")==NULL) xQueueSend(sim_at_queue_handle,data,portMAX_DELAY);
        // if (strchr(data,'>')){
        //     send_posible=true;
        //     printf("detected >\r\n");
        // }
        send_posible=true;
        // else if(strstr(data,"AT+QMTPUBEX=1,0,0,0")){
        //     vTaskDelay(10/portTICK_PERIOD_MS);
        //     send_posible=true;
        // }
     }
   //  vTaskDelay(10/portTICK_PERIOD_MS);
   }
   
}
// #define BUF_SIZE_DEVICE (1050)
// #define RD_BUF_SIZE_DEVICE (BUF_SIZE_DEVICE)

// void read_and_send_to_queue_task(void *pvParameters)
// {
//     uint8_t *dtmp_device = (uint8_t *)malloc(RD_BUF_SIZE_DEVICE);
//     if (!dtmp_device)
//         return;

//     int total_len = 0;                                    // số byte đã nhận được tổng cộng
//     sim_at_queue_handle = xQueueCreate(10, BUF_SIZE_SIM); // 10: có thể chứa tối đa 10 phần tử, BUF_SIZE_SIM: kích thước của mỗi phần tử
//     mqtt_queue_handle = xQueueCreate(10, BUF_SIZE_SIM);
//     gps_queue_handle = xQueueCreate(10, BUF_SIZE_SIM);
//     publish_queue_handle = xQueueCreate(10, BUF_SIZE_SIM);

//     while (1)
//     {
//         // đọc 1 byte mỗi lần, timeout 1000ms
//         int rxBytes_device = uart_read_bytes(UART_SIM_NUM, dtmp_device + total_len, 1, 1000 / portTICK_PERIOD_MS);

//         if (rxBytes_device > 0)
//         {
//             total_len += rxBytes_device;

//             // nếu gần đầy buffer thì reset về 0 để tránh tràn
//             if (total_len >= RD_BUF_SIZE_DEVICE - 1)
//             {
//                 total_len = 0;
//                 printf("⚠ Buffer đầy, reset lại!\n");
//                 if (total_len > 0)
//                 {
//                     dtmp_device[total_len] = '\0';
//                     // printf("Queue RX: %s\r\n",data);
//                     if (strstr((const char *)dtmp_device, "+QMTRECV:") != NULL){
//                         xQueueSend(mqtt_queue_handle, dtmp_device, portMAX_DELAY); //
//                         printf("recieved");
//                     }
//                     // else if(strstr(data,"+QGPSLOC:")!=NULL) xQueueSend(gps_queue_handle,data,portMAX_DELAY);
//                     else
//                         xQueueSend(sim_at_queue_handle, dtmp_device, portMAX_DELAY);
//                 }
//             }
//             if (strchr((const char *)dtmp_device, '>'))
//             {
//                 send_posible = true;
//                 //printf("detected >\r\n");
//             }

//             // thêm ký tự kết thúc chuỗi để in ra an toàn
//             dtmp_device[total_len] = '\0';

//          //   printf("UART Data (%d bytes): ", total_len);
//             // for (int i = 0; i < total_len; i++)
//             // {
//             //     printf("%02X ", dtmp_device[i]); // in dạng HEX
//             // }
//             // printf("\n");
//         }
//     }

//     free(dtmp_device);
//     vTaskDelete(NULL);
// }