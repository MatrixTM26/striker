idf_component_register(
    SRCS "RadioCore.cpp" "RadioScanner.cpp" "RadioSniffer.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event esp_netif nvs_flash
)
