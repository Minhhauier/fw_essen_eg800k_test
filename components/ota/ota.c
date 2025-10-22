#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "esp_ota_ops.h"
#include "esp_system.h"

#include "ota.h"
// #include "uart.h"
// #include "at_command.h"
#include "system_manage.h"
#include "config_parameter.h"
#include "mqtt.h"



static uint8_t data[BUF_SIZE_SIM];
esp_ota_handle_t ota_handle = 0;
const esp_partition_t *update_partition = NULL;

void send_read_at(const char *cmd, int timeout) {
    printf("Sent: %s\r\n", cmd);
    int delay=10;
    uart_write_bytes(UART_SIM_NUM, cmd, strlen(cmd));
    uart_write_bytes(UART_SIM_NUM,"\r\n",2);
    int total_len=0;
    vTaskDelay(1000/portTICK_PERIOD_MS);
    for(int i=0;i<timeout;i=i+10){
        int len = uart_read_bytes(UART_SIM_NUM,data+total_len,BUF_SIZE_SIM-total_len-1,10/portTICK_PERIOD_MS);
        if(len>0) total_len=total_len+len;
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    if(total_len>0){
        data[total_len]='\0';
        printf("Rx: %s\r\n",data);
    }
    else{
        printf("No respone\r\n");
    }
}
long get_length_file(const char* file_link, int timeout)
{
    char cmd[128];
    //send_at("AT+HTTPTERM\r\n", 1000);
  //  send_at("AT+QHTTPSTOP",1000);
    send_read_at("AT+QHTTPCFG=\"responseheader\",0",1000); //AT+QHTTPSTOP
    send_read_at("AT+QIACT=1\r\n",1000);//AT+QIACT=1 
    snprintf(cmd,sizeof(cmd),"AT+QHTTPURL=%d,80",strlen(file_link));
    send_read_at(cmd,1000);
    send_read_at(file_link,1000);
   // snprintf(cmd, sizeof(cmd),"AT+HTTPPARA=\"URL\",\"%s\"",file_link);// AT+QHTTPURL=46,80 => http://ota.evsafe.vn/dev/spi_lcd_touch_RAM.bin
    //send_at(cmd,1000);
    uart_write_bytes(UART_SIM_NUM, "AT+QHTTPGET=80\r\n", strlen("AT+QHTTPGET=80\r\n"));
    uint8_t data[BUF_SIZE_SIM];
    memset(data,0,BUF_SIZE_SIM);
    int total_len=0;
    for(int i=0;i<timeout;i=i+100)
    {
        int len = uart_read_bytes(UART_SIM_NUM,data+total_len,BUF_SIZE_SIM-total_len-1,100/portTICK_PERIOD_MS);
        if(len>0) total_len=total_len+len;
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    if(total_len>0) {
        data[total_len]='\0';
        printf("RX:%s\r\n",data);
    }
    else printf("No respone");
    vTaskDelay(1000/portTICK_PERIOD_MS);
   // send_at("AT+QHTTPSTOP",1000);
    long size=0;
    char value[64];
    char *start = strstr((char*)data,"+QHTTPGET: 0,200,");
    if (start==NULL) {printf("data failed: %s\r\n",data);return -1;}
    char *end = strchr(start,'\n');
    if (end==NULL) {printf("data failed: %s\r\n",data);return -1;}
    start=start+strlen("+QHTTPGET: 0,200,");
    int len = end-start;
    memcpy(value,start,len);
    value[len] = '\0';
    size = atol(value);
    printf("data: %s\r\n",data);
    return size;
}

void get_data(char *cmd, int timeout,long size_file){
    printf("Sent: %s\r\n",cmd);
    int total_len=0;
    int count=0;
    bool check_header=false;
    uart_write_bytes(UART_SIM_NUM,cmd,strlen(cmd));
    while (total_len<size_file && timeout>0)
    {
      int len = uart_read_bytes(UART_SIM_NUM,data,2048-1,100/portTICK_PERIOD_MS);
      int offset=0;
      if(len>0){
        if(check_header==false){
            char *start_data=strstr((char*)data,"CONNECT\r\n");
            if(start_data!=NULL){
                offset = (start_data-(char*)data)+strlen("CONNECT\r\n");
                len = len - offset;
                check_header=true;
            }
            else {
                printf("Không có header được tìm thấy \r\n");
                continue;
            }
        }
        total_len=total_len+len;
        if(total_len>size_file){
         len=len-(total_len-size_file);
         total_len=size_file;
        }
        esp_ota_write(ota_handle,&data[offset],len);
        printf("Wrote %d/%ld bytes\r\n",total_len,size_file);
        count=0;
      }
      vTaskDelay(10/portTICK_PERIOD_MS);
      timeout=timeout-10;
    }
    if(total_len==size_file) printf("OTA finish\r\n");
    else {
        printf("2.write file failed\r\n");
        printf("time out: %d\r\n",timeout);
    }
}
void OTA_from_link_v2(char *file_link, int timeout){
    char cmd[256];
    long size_file=get_length_file(file_link,5000);
    int count=0;
    while(size_file==-1)
    {
        printf("check size file, check again!! \r\n");
        size_file=get_length_file(file_link,5000);
        count++;
        if(count>5) break;
        // vTaskDelay(2000/portTICK_PERIOD_MS);
    }
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        printf("No OTA partition found!\n");
        return;
    }
    if (esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle) != ESP_OK) {
        printf("esp_ota_begin failed!\n");
        return;
    }
    printf("file size: %ld\r\n",size_file);
    get_data("AT+QHTTPREAD=80\r\n",300000,size_file);
    send_read_at("AT+QHTTPSTOP",1000);
    if(esp_ota_end(ota_handle)==ESP_OK){
         printf("OTA write success!!\n");
         if (esp_ota_set_boot_partition(update_partition)==ESP_OK){
            printf("OTA success, rebooting....\r\n");
            publish_version(HW_VERSION,FW_VERSION,1);
         }
         else {
            printf("OTA failed...\r\n");
            //publish_version(HW_VERSION,FW_VERSION,0);
         }
    }
    else {
        printf("OTA write failed!!");
        //publish_version("2.0","2.0",1);
    }
}