idf_component_register(
    SRCS "src/rfc2217_server.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES lwip pthread
)
