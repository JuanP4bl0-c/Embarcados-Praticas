idf_component_register(SRCS "main.c" 
                            "wifi_manager.c" 
                            "mqtt_manager.c" 
                            "uv_sensor.c"
                            "soil_moisture.c"
                            "dht11_sensor.c"
                            "solenoid.c"
                            "plant_config.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi esp_event esp_netif mqtt lwip esp_driver_gpio esp_timer driver esp_adc
                    EMBED_TXTFILES certs/AmazonRootCA1.pem
                                  certs/86cf2f23eee12460caeaf6c97ae117032f556f433959a2bfd83039ab5fa3ae0c-certificate.pem.crt
                                  certs/86cf2f23eee12460caeaf6c97ae117032f556f433959a2bfd83039ab5fa3ae0c-private.pem.key)
