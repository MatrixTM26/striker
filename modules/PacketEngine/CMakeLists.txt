idf_component_register(
    SRCS
        "PacketEngine.cpp"
        "StrikeMethod.cpp"
        "StrikePmkid.cpp"
        "StrikeHandshake.cpp"
        "StrikeDisrupt.cpp"
    INCLUDE_DIRS "." ".."
    REQUIRES esp_event esp_timer esp_wifi
    PRIV_REQUIRES RadioInterface FrameParser CaptureWriter FrameInjector
)
