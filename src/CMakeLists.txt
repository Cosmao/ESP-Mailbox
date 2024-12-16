idf_component_register(SRCS "main.c" "deep_sleep.c" "wifi.c" "mqtt.c"
                    INCLUDE_DIRS ""
                    REQUIRES mbedtls
                    EMBED_TXTFILES
                    ../certs/mosquitto.org.crt
                    ../certs/client.key
                    ../certs/client.crt)

