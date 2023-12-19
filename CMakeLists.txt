set(component_srcs src/esp_idf_ssd1306.c src/esp_idf_ssd1306.c src/esp_idf_ssd1306.c)

idf_component_register(SRCS "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS "./include")
