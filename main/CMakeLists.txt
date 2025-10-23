idf_component_register(SRCS "main.c" "system_manage.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        driver
                        config
                        mqtt
                        pzem
                        mqtt
                        #encrypt
                        control_led
                        drivers
                        #control_relay
                        gps_eg800k
                    )
