#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "at_command.h"
#include "mqtt.h"
#include "config_parameter.h"
#include "encrypt_decrypt.h"

static char data[BUF_SIZE_SIM];
void gps_init(){
    send_at("AT+QGPSCFG=\"outport\",\"usbnmea\"");
    vTaskDelay(500/portTICK_PERIOD_MS);
    send_at("AT+QGPS=1");
    vTaskDelay(500/portTICK_PERIOD_MS);
}
//if(xQueueReceive(mqtt_queue_handle,data,pdMS_TO_TICKS(50))==pdTRUE)

char *get_sub_arr(char *dt,char *start,char end){
    char *s = strstr(dt,start)+strlen(start);
    if(s==NULL){
        printf("start null\r\n");
        return NULL;
    }
    char *e = strchr(dt,end);
    if(e==NULL){
        printf("end null\r\n");
        return NULL;
    }
    int len = e-s;
 //   printf("len: %d\r\n",len);
    char *sub_data = calloc(len+1,sizeof(char));
    memcpy(sub_data,s,len);
    sub_data[len]='\0';
    return sub_data;
}

void gps_process_data(char *dt){
    //char * dt = "+QGPSLOC: 063416.40,3143.2951N,11713.0655E,0.6,224.9,2,162.57,17.6,9.5,110620,07";
    //printf("data: %s\r\n",data);
    char *lat = get_sub_arr(dt,",",'N');
    char *lng = get_sub_arr(dt,"N,",'E');
    float latitude = atof(lat);
    float longitude = atof(lng);
    printf("Lat: %f - Lng: %f\r\n",latitude,longitude);
    publish_gpsposition(latitude,longitude);
    free(lat);free(lng); 
}