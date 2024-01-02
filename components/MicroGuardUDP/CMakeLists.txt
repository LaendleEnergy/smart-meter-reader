idf_component_register(SRCS "micro_guard_udp.c"
                    INCLUDE_DIRS "include"
                    REQUIRES mbedtls nvs_flash utils)
