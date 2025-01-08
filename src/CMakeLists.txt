idf_component_register(SRCS "main.c" "deep_sleep.c" "wifi.c" "mqtt.c"
                    INCLUDE_DIRS ""
                    REQUIRES mbedtls
                    EMBED_TXTFILES
                    ../certs/ca.crt
                    ../certs/esp32.crt
                    ../certs/esp32.key
                    )

