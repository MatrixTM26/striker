idf_component_register(
    SRCS "FrameInjector.cpp"
    INCLUDE_DIRS "."
)
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
