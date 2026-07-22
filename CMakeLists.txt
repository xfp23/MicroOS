project(MicroOS C)

file(GLOB_RECURSE MicroOS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(MicroOS STATIC
    ${MicroOS_SOURCES}
)

target_include_directories(MicroOS PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# GCC 编译选项
target_compile_options(MicroOS PRIVATE
    "$<$<C_COMPILER_ID:GNU>:-mcpu=cortex-m4>" # 选择目标cpu架构
    "$<$<C_COMPILER_ID:GNU>:-mthumb>"
    "$<$<C_COMPILER_ID:GNU>:-Wall>"
)