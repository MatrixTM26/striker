idf_component_register(
    SRCS
        "PacketEngine.cpp"
        "StrikeMethod.cpp"
        "StrikePmkid.cpp"
        "StrikeHandshake.cpp"
        "StrikeDisrupt.cpp"
    INCLUDE_DIRS "." ".."
    PRIV_REQUIRES RadioInterface FrameParser CaptureWriter FrameInjector
)
