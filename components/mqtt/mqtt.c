#include <stdio.h>
#include <stdlib.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#include "config_parameter.h"
#include "at_command.h"
#include "mqtt.h"
#include "encrypt_decrypt.h"
#include "system_manage.h"

static char addr[30] = "broker.chtlab.us,1883";
static char password[30] = "password";
static char username[30] = "username";
static char client[50];
// static char *sub_topic[]={"SU/SmartEVsafe"};
static char pub_topic[30];
static char cmd[256];

#define MQTT_TAG "MQTT"

bool mqtt_sub_success = false;
bool read_enable = false;

static char json[1024];
static char buffer[1024];
static char client_id[50];

void mqtt_publish_encrypted(char *data, char *serial_number, int cmd_type)
{
    char *json_entrypted = encrypt_data(data, serial_number, cmd_type);
    snprintf(cmd, 256, "%s/SmartEVsafe", PUB);
    mqtt_pub(cmd, json_entrypted);
    free(json_entrypted);
    free(data);
}

void mqtt_connect()
{
    send_at_get_respond("AT+QMTCFG=\"keepalive\",1,60", 1000);
    snprintf(cmd, sizeof(cmd), "AT+QMTOPEN=1,%s", addr);
    send_at_get_respond(cmd, 1000);
    snprintf(client, 50, "%s_%s", DEVICE_NAME, device_name);
    snprintf(cmd, sizeof(cmd), "AT+QMTCONN=1,\"%s\",[\"%s\",\"%s\"]", client, username, password);
    send_at_get_respond(cmd, 1000);
}

void mqtt_sub()
{
    snprintf(client_id, 50, "%s_%s", DEVICE_NAME, device_name); // EVsafe_EV4f00a2bf04
    // snprintf(cmd,sizeof(cmd),"AT+QMTSUB=1,1,%s/SmartEVsafe,0,%s,0",SUB,client_id);
    snprintf(cmd, sizeof(cmd), "AT+QMTSUB=1,1,EVsafe_EV4f00a2bf04,0,%s,0", client_id);
    send_at(cmd);
    char *data = get_respond(1000);
    if (data != NULL)
    {
        if (strstr(data, "+QMTSUB: 1,1,0,0,0") != NULL)
            mqtt_sub_success = true;
        else
            mqtt_sub_success = false;
    }
    else
    {
        mqtt_sub_success = false;
        printf("No data respond\r\n");
    }
    free(data);
}
void mqtt_unsub()
{
    snprintf(client_id, 50, "%s_%s", DEVICE_NAME, device_name);
    snprintf(cmd, sizeof(cmd), "AT+QMTUNS=1,1,%s/SmartEVsafe,0,%s,0", SUB, client_id);
    send_at_get_respond(cmd, 2000);
}
void mqtt_pub(char *topic, char *payload)
{
    snprintf(cmd, sizeof(cmd), "AT+QMTPUBEX=1,0,0,0,\"%s\",%d", topic, strlen(payload));
    // send_at_get_respond(cmd,1000);
    // send_at_get_respond(payload,1000);
    // printf("cmd: %s\r\n",cmd);
    // printf("payload: %s\r\n",payload);

    int timeout = 1000;
    send_at(cmd);
    if (read_enable)
    {
        while (timeout > 0 && send_posible == false)
        {
            timeout = timeout - 100;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        if (send_posible)
        {
            // printf("da gui\r\n");
            send_at(payload);
            send_posible = false;
        }
    }
    else
    {
        send_at(payload);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
}
void mqtt_init()
{
    read_enable = false;
    int count = 0;
    while (mqtt_sub_success == false)
    {
        count = count + 100;
        mqtt_connect();
        mqtt_sub();
        if (count > 500)
        {
            count = 0;
            send_at("AT+CFUN=1,1");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    read_enable = true;
    ESP_LOGI(MQTT_TAG, "Connect and subscribe to mqtt success");
}
void comunicate_with_mqtt_task()
{
    mqtt_init();
    while (1)
    {
        char *data = get_respond(1000);
        printf("RX_SIM: %s\r\n", data);
    }
}

// cmd_type:205_all
void publish_data_power(float *value_power, int *value_vol)
{
    snprintf(json, sizeof(json), "{\n"
                                 "  \"data\": {\n"
                                 "    \"gun1\": { \"power\": %f,\"vol\": %d },\n"
                                 "    \"gun2\": { \"power\": %f,\"vol\": %d },\n"
                                 "    \"gun3\": { \"power\": %f,\"vol\": %d },\n"
                                 "    \"gun4\": { \"power\": %f,\"vol\": %d },\n"
                                 "    \"gun5\": { \"power\": %f,\"vol\": %d },\n"
                                 "    \"gun6\": { \"power\": %f,\"vol\": %d }\n"
                                 "  }\n"
                                 "}",
             value_power[0], value_vol[0],
             value_power[1], value_vol[1],
             value_power[2], value_vol[2],
             value_power[3], value_vol[3],
             value_power[4], value_vol[4],
             value_power[5], value_vol[5]);
    char *json_encrypted = encrypt_data(json, device_name, 205);
    if (json_encrypted == NULL)
    {
        printf("205: Can't encrypted, mqtt publish failed\r\n");
    }
    else
    {
        // mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer, json_encrypted);
        xQueueSend(publish_queue_handle, buffer, portMAX_DELAY);
    }
    free(json_encrypted);
}
// cmd_type: 201
void publish_version(char *verHW, char *verFW, int status)
{
    snprintf(json, sizeof(json), "{\n"
                                 "  \"vesion\": {\n"
                                 "    \"verFW\": \"%s\", \"verHW\": \"%s\", \"status\":%d\n"
                                 "  }\n"
                                 "}",
             verFW, verHW, status);
    char *json_encrypted = encrypt_data(json, device_name, 201);
    if (json_encrypted == NULL)
    {
        printf("201: Can't encrypted, mqtt publish failed\r\n");
    }
    else
    {
        if (read_enable)
        {
            strcpy(buffer, json_encrypted);
            xQueueSend(publish_queue_handle, buffer, portMAX_DELAY);
        }
        else
            mqtt_pub("UP4G/SmartEVsafe", json_encrypted);
    }
    free(json_encrypted);
}
// cmd_type: 202
void publish_gpsposition(float latitude_decimal, float longitude_decimal)
{
    snprintf(json, sizeof(json), "{\n"
                                 "  \"gps\": {\n"
                                 "    \"longitude\": %f,\n"
                                 "    \"latitude\": %f\n"
                                 "  }\n"
                                 "}",
             latitude_decimal, longitude_decimal);
    char *json_encrypted = encrypt_data(json, device_name, 202);
    if (json_encrypted == NULL)
    {
        printf("202: Can't encrypted, mqtt publish failed\r\n");
    }
    else
    {
        strcpy(buffer, json_encrypted);
        xQueueSend(publish_queue_handle, buffer, portMAX_DELAY);
    }
    free(json_encrypted);
}
// cmd_type: 203
void publish_temp(double temp)
{
    snprintf(json, sizeof(json), "{\n"
                                 "  \"data\": {\n"
                                 "    \"temp\": %f\n"
                                 "  }\n"
                                 "}",
             temp);
    char *json_encrypted = encrypt_data(json, device_name, 203);
    if (json_encrypted == NULL)
    {
        printf("203: Can't encrypted, mqtt publish failed\r\n");
    }
    else
    {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer, json_encrypted);
        xQueueSend(publish_queue_handle, buffer, portMAX_DELAY);
    }
    free(json_encrypted);
}
// cmd_type: 204
//  void publish_errorcode(int error,int port_num){
//      char *json = make_json_errorcode(error,port_num);
//      mqtt_publish_encrypted(json,device_name,204);
//      free(json);
//  }
// cmd_type:205
void publish_data_1gun_only(int gun_number, float power, float vol)
{
    snprintf(json, sizeof(json), "{\n"
                                 "  \"data\": {\n"
                                 "    \"gun%d\": {\n"
                                 "      \"power\": %f,\n"
                                 "      \"vol\": %f\n"
                                 "    }\n"
                                 "  }\n"
                                 "}",
             gun_number, power, vol);
    char *json_encrypted = encrypt_data(json, device_name, 205);
    if (json_encrypted == NULL)
    {
        printf("205: Can't encrypted, mqtt publish failed\r\n");
    }
    else
    {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer, json_encrypted);
        xQueueSend(publish_queue_handle, buffer, portMAX_DELAY);
    }
    free(json_encrypted);
}
// cmd_type: 206
void publish_wifi_infor(char *ssid, char *mac, char *ip, int rssi)
{
    snprintf(json, sizeof(json), "{\n"
                                 "  \"SSID\": \"%s\",\n"
                                 "  \"MAC\": \"%s\",\n"
                                 "  \"IP\": \"%s\",\n"
                                 "  \"Rssi\": %d\n"
                                 "}",
             ssid, mac, ip, rssi);
    char *json_encrypted = encrypt_data(json, device_name, 206);
    if (json_encrypted == NULL)
    {
        printf("206: Can't encrypted, mqtt publish failed\r\n");
    }
    else
    {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer, json_encrypted);
        xQueueSend(publish_queue_handle, buffer, portMAX_DELAY);
    }
    free(json_encrypted);
}
// cmd_tyoe: 207
// 1-idle 2-charging 3-error 4-complete
void publish_gun_status(int *value_status)
{
    snprintf(json, sizeof(json), "{\n"
                                 "  \"data\" : {\n"
                                 "    \"gun1\" : {\n"
                                 "      \"status\" : %d\n"
                                 "    },\n"
                                 "    \"gun2\" : {\n"
                                 "      \"status\" : %d\n"
                                 "    },\n"
                                 "    \"gun3\" : {\n"
                                 "      \"status\" : %d\n"
                                 "    },\n"
                                 "    \"gun4\" : {\n"
                                 "      \"status\" : %d\n"
                                 "    },\n"
                                 "    \"gun5\" : {\n"
                                 "      \"status\" : %d\n"
                                 "    },\n"
                                 "    \"gun6\" : {\n"
                                 "      \"status\" : %d\n"
                                 "    }\n"
                                 "  }\n"
                                 "}",
             value_status[0], value_status[1], value_status[2],
             value_status[3], value_status[4], value_status[5]);
    char *json_encrypted = encrypt_data(json, device_name, 207);
    if (json_encrypted == NULL)
    {
        printf("207: Can't encrypted, mqtt publish failed\r\n");
    }
    else
    {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer, json_encrypted);
        xQueueSend(publish_queue_handle, buffer, portMAX_DELAY);
    }
    free(json_encrypted);
}
void respond_to_mqtt(int gate, int state, int cmd)
{
    snprintf(json, 1024,
             "{\n"
             "  \"data\": {\n"
             "    \"V\": %d,\n"
             "    \"S\": %d,\n"
             "    \"Cmd\": %d\n"
             "  }\n"
             "}",
             gate, state, cmd);
    // char *json_encrypted = encrypt_data(json,device_name,208);
    char *json_encrypted = encrypt_data(json, "EV4f00a2bf04", 208);
    // malloc + copy riêng

    if (json_encrypted == NULL)
    {
        printf("208: Can't encrypted, mqtt publish failed\r\n");
    }
    else
    {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer, json_encrypted);
        xQueueSend(publish_queue_handle, buffer, portMAX_DELAY);
        // printf("data %s\r\n",buffer);
    }

    // printf("data encrtypt: %s\r\n",json_encrypted);
    free(json_encrypted);
    // mqtt_publish_encrypted(json, device_name, 208);
}