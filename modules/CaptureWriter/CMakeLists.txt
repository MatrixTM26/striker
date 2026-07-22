idf_component_register(
    SRCS "PcapWriter.cpp" "HccapxWriter.cpp"
    INCLUDE_DIRS "." ".."
    PRIV_REQUIRES FrameParser
)
