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
                            "system_commands.c"
                            "power_manager.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi esp_event esp_netif mqtt lwip esp_driver_gpio esp_timer driver esp_adc esp_pm
                    EMBED_TXTFILES certs/AmazonRootCA1.pem
                                  certs/376f19f7d489fd831039a918bc7a9ec29a363566a92e0c10b4fc5b0f69aa345f-certificate.pem.crt
                                  certs/376f19f7d489fd831039a918bc7a9ec29a363566a92e0c10b4fc5b0f69aa345f-private.pem.key)
