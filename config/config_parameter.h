#ifndef CONFIG_PARAMETER
#define CONFIG_PARAMETER

#include<driver/uart.h>
#include<driver/gpio.h>

//chose mode for device
#define DEVICE_PROD             0 //1.0.1
#define DEVICE_DEV              1 //0.0.1

#define DEVICE_MODE            DEVICE_DEV 

#if DEVICE_MODE == DEVICE_DEV
    #define DEVICE_NAME         "EVsafe"
    #define PUB                 "UP4G"
    #define SUB                 "SU4G"
#elif DEVICE_MODE == DEVICE_PROD
    #define DEVICE_NAME         "PEVsafe"
    #define PUB                 "PUP4G"
    #define SUB                 "PSU4G"
#endif

//config parameter for uart
#define UART_SIM_NUM            UART_NUM_1
#define TX_SIM                  17
#define RX_SIM                  39
#define UART_PZEM_NUM          UART_NUM_2
#define TX_PZEM                 27
#define RX_PZEM                 36
#define UART_BAUD_RATE          115200
#define BUF_SIZE_SIM            4096
#define BUF_SIZE_PZEM           1024

//config parameter for relay
#define GATE_NUM_1              GPIO_NUM_23
#define GATE_NUM_2              GPIO_NUM_22
#define GATE_NUM_3              GPIO_NUM_19
#define GATE_NUM_4              GPIO_NUM_18
#define GATE_NUM_5              GPIO_NUM_26
#define GATE_NUM_6              GPIO_NUM_4

#define RELAY_SLCH              GPIO_NUM_25
#define RELAY_SLCK              GPIO_NUM_33
#define RELAY_SDAT              GPIO_NUM_32

//config parameter for led
#define CHARGE_LED_GPIO          15
#define CHARGE_LED_NUMBER        18
#define CHARGE_LED_LEN           3

#define CONNECTION_LED_GPIO      1
#define CONNECTION_LED_NUMBER    1
#define CONNECTION_LED_LEN       1

#endif