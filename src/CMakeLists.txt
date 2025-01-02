idf_component_register(SRCS "main.c" "deep_sleep.c" "wifi.c" "mqtt.c" "ultrasonic_distance.c"
                    INCLUDE_DIRS ""
                    REQUIRES mbedtls
                    EMBED_TXTFILES
                    ../certs/AmazonRootCA1.pem
                    ../certs/client.pem.key
                    ../certs/client.pem.crt)

