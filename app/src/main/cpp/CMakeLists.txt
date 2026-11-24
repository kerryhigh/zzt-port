cmake_minimum_required(VERSION 3.22.1)
project(zztengine C)

set(LIBZOO "${CMAKE_CURRENT_LIST_DIR}/../../../../third_party/libzoo")
find_package(Python3 REQUIRED COMPONENTS Interpreter)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/libzoo")
set(OOP_TOKEN_C "${CMAKE_CURRENT_BINARY_DIR}/libzoo/zoo_oop_token.c")
add_custom_command(
    OUTPUT "${OOP_TOKEN_C}"
    COMMAND "${Python3_EXECUTABLE}" "${LIBZOO}/tools/tok2c.py"
            "${LIBZOO}/src/libzoo/zoo_oop_token.tok" "${OOP_TOKEN_C}"
    DEPENDS "${LIBZOO}/tools/tok2c.py" "${LIBZOO}/tools/tok2c_tpl.c"
            "${LIBZOO}/src/libzoo/zoo_oop_token.tok"
    VERBATIM
)
add_custom_target(zzt_oop_tokens DEPENDS "${OOP_TOKEN_C}")

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${LIBZOO}/src/libzoo/*.c"
    "${LIBZOO}/src/ui/*.c"
    "${LIBZOO}/src/drivers/zoo_io_path.c"
    "${LIBZOO}/src/drivers/zoo_io_posix.c"
)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/(zoo_ui_(debug|osk)|zoo_oop_label_cache)\\.c$")
add_library(zztengine SHARED
    native-lib.c
    "${LIBZOO}/src/frontend/sdl/src/8x14.c"
    "${CMAKE_CURRENT_LIST_DIR}/zzt_sound_pcm.c"
    ${ENGINE_SOURCES}
)
add_dependencies(zztengine zzt_oop_tokens)
target_include_directories(zztengine PRIVATE
    "${LIBZOO}/include"
    "${LIBZOO}/src"
    "${LIBZOO}/src/frontend/sdl/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
target_compile_features(zztengine PRIVATE c_std_99)
target_compile_definitions(zztengine PRIVATE
    ZOO_OPTIMIZE
    ZOO_CONFIG_ENABLE_EDITOR_CONSTANTS
    ZOO_CONFIG_USE_DOUBLE_FOR_MS
)
target_compile_options(zztengine PRIVATE -Wall -Wno-unused-parameter -Wno-unused-function)
target_link_libraries(zztengine PRIVATE m)
