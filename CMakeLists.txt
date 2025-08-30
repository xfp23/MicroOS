file(GLOB SRC_FILES "./src/*.c")

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS "./include"
)

# 设置 C 标准为 C99
target_compile_options(${COMPONENT_LIB} PRIVATE "-std=c99")
