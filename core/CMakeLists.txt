idf_component_register(
    SRCS "Main.cpp"
    INCLUDE_DIRS "." "../modules/RadioInterface" "../modules/PacketEngine" "../modules/HttpServer"
    REQUIRES esp_event
    PRIV_REQUIRES RadioInterface PacketEngine HttpServer
)
