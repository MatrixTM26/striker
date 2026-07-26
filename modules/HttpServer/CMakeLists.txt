idf_component_register(
    SRCS "HttpServer.cpp"
    INCLUDE_DIRS "." ".."
    REQUIRES esp_event esp_http_server
    PRIV_REQUIRES RadioInterface PacketEngine CaptureWriter
)
