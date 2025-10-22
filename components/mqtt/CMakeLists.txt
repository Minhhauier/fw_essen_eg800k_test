file(GLOB SRCS "*.c")

idf_component_register(
        SRCS ${SRCS}
        INCLUDE_DIRS include
        REQUIRES
            driver
            freertos
            config
            at_command
            encrypt
        PRIV_REQUIRES
            main
)
