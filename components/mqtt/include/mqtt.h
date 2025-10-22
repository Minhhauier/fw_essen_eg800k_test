#ifndef MQTT
#define MQTT

extern char device_name[25];
extern bool mqtt_sub_success;
extern bool read_enable;
extern bool pzem_read_enable;


extern bool send_posible;
void mqtt_init();
void mqtt_sub();
void mqtt_pub(char *topic,char *payload);
void comunicate_with_mqtt_task();
void mqtt_connect();
void publish_data_power(float *value_power,int *value_vol);
void publish_version(char *verHW,char *verFW,int status);
void publish_temp(double temp);
void publish_data_1gun_only(int gun_number,float power,float vol);
void publish_wifi_infor(char *ssid,char *mac,char*ip,int rssi);
void publish_gun_status(int *value_status);
void respond_to_mqtt(int gate, int state, int cmd);
void publish_gpsposition(float latitude_decimal,float longitude_decimal);
void mqtt_unsub();
#endif