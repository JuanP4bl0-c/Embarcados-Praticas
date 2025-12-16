idf_component_register(SRCS "main.c" 
                            "wifi_manager.c" 
                            "mqtt_manager.c" 
                            "uv_sensor.c"
                            "soil_moisture.c"
                            "dht11_sensor.c"
                            "solenoid.c"
                            "plant_config.c"
                            "day_night_control.c"
                            "ntp_sync.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi esp_event esp_netif mqtt lwip esp_driver_gpio esp_timer driver esp_adc
                    EMBED_TXTFILES certs/AmazonRootCA1.pem
                                  certs/784b1c21017ae276cbea71bc6b3ed0d1fa6377bb594ee4094f9271ff367ecc3e-certificate.pem.crt
                                  certs/784b1c21017ae276cbea71bc6b3ed0d1fa6377bb594ee4094f9271ff367ecc3e-private.pem.key)
