#ifndef SYSTEM_MANAGE
#define SYSTEM_MANAGE

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// #include <config_parameter.h>

// #define DEVICE_PROD             0 //1.0.1
// #define DEVICE_DEV              1 //0.0.1

extern bool send_posible;
#if DEVICE_MODE == DEVICE_DEV
    #define HW_VERSION             "0.1"
    #define FW_VERSION             "0.0.1"
#elif DEVICE_MODE == DEVICE_PROD
    #define HW_VERSION             "0.1"
    #define FW_VERSION             "1.0.1"
#endif

extern TaskHandle_t sim_task;
extern TaskHandle_t pzem_ds18b20_task;
extern TaskHandle_t read_sim_uart_task;

/**
 * @brief all processes with SIM is demonstrated in this function
 */
void sim_mqtt_task(void *pvParameters);

#endif