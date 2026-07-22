idf_component_register(
    SRCS "HttpServer.cpp"
    INCLUDE_DIRS "." ".."
    PRIV_REQUIRES RadioInterface PacketEngine CaptureWriter esp_http_server
)
