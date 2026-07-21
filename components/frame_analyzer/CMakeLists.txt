idf_component_register(
    SRCS "frame_analyzer.cpp" "frame_analyzer_parser.cpp"
    INCLUDE_DIRS "interface"
    PRIV_REQUIRES wifi_controller
)
