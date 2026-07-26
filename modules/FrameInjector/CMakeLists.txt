idf_component_register(
    SRCS "FrameInjector.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi
)
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
