idf_component_register(
    SRCS "FrameParser.cpp" "FrameDecoder.cpp"
    INCLUDE_DIRS "." ".."
    PRIV_REQUIRES RadioInterface
)
