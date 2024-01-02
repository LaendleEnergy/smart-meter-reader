idf_component_register(SRCS "mbus_minimal.c" "kaifa.c" "dlms.c" 
                    INCLUDE_DIRS "include"
                    REQUIRES mbedtls driver)