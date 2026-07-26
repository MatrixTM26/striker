idf_component_register(
    SRCS "FrameParser.cpp" "FrameDecoder.cpp"
    INCLUDE_DIRS "." ".."
    REQUIRES esp_event esp_wifi
    PRIV_REQUIRES RadioInterface
)
