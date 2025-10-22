file(GLOB SRCS "*.c")

idf_component_register(
        SRCS ${SRCS}
        INCLUDE_DIRS include
        REQUIRES
            driver
            freertos
            config
            mbedtls
            json

            drivers
            mqtt
            control_led
            control_relay
            ota
        PRIV_REQUIRES
            main    
            
)
