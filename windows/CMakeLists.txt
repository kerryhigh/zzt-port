cmake_minimum_required(VERSION 3.22)
project(ZZTWindows LANGUAGES C)

if(NOT WIN32)
	message(FATAL_ERROR "The native Windows frontend must be built on Windows.")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(LIBZOO "${CMAKE_CURRENT_LIST_DIR}/../third_party/libzoo")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")
file(MAKE_DIRECTORY "${GENERATED_DIR}/libzoo")
set(OOP_TOKEN_C "${GENERATED_DIR}/libzoo/zoo_oop_token.c")

add_custom_command(
	OUTPUT "${OOP_TOKEN_C}"
	COMMAND "${Python3_EXECUTABLE}" "${LIBZOO}/tools/tok2c.py"
		"${LIBZOO}/src/libzoo/zoo_oop_token.tok" "${OOP_TOKEN_C}"
	DEPENDS "${LIBZOO}/tools/tok2c.py" "${LIBZOO}/tools/tok2c_tpl.c"
		"${LIBZOO}/src/libzoo/zoo_oop_token.tok"
	VERBATIM
)

set(LIBZOO_SOURCES
	${LIBZOO}/src/libzoo/zoo_callstack.c
	${LIBZOO}/src/libzoo/zoo_editor.c
	${LIBZOO}/src/libzoo/zoo_elements.c
	${LIBZOO}/src/libzoo/zoo_game_io.c
	${LIBZOO}/src/libzoo/zoo_game.c
	${LIBZOO}/src/libzoo/zoo_high_scores.c
	${LIBZOO}/src/libzoo/zoo_editor_brd.c
	${LIBZOO}/src/libzoo/zoo_input.c
	${LIBZOO}/src/libzoo/zoo_io.c
	${LIBZOO}/src/libzoo/zoo_oop.c
	${LIBZOO}/src/libzoo/zoo_sound.c
	${LIBZOO}/src/libzoo/zoo_window.c
	${LIBZOO}/src/libzoo/zoo_window_classic.c
	${LIBZOO}/src/libzoo/zoo.c
	${LIBZOO}/src/ui/zoo_sidebar_classic.c
	${LIBZOO}/src/ui/zoo_ui.c
	${LIBZOO}/src/ui/zoo_ui_file_select.c
	${LIBZOO}/src/ui/zoo_ui_input.c
	${LIBZOO}/src/ui/zoo_ui_prompt.c
	${LIBZOO}/src/ui/zoo_ui_util.c
	${LIBZOO}/src/drivers/zoo_io_path.c
	${LIBZOO}/src/drivers/zoo_sound_pcm.c
	${LIBZOO}/src/frontend/sdl/src/8x14.c
	${LIBZOO}/src/frontend/sdl/src/render_software.c
	${OOP_TOKEN_C}
)

add_library(zzt_core STATIC ${LIBZOO_SOURCES})
target_include_directories(zzt_core PUBLIC
	"${LIBZOO}/include"
	"${LIBZOO}/src"
	"${LIBZOO}/src/frontend/sdl/src"
	"${GENERATED_DIR}"
)
target_compile_features(zzt_core PUBLIC c_std_99)
target_compile_definitions(zzt_core PRIVATE
	ZOO_OPTIMIZE
	ZOO_CONFIG_ENABLE_EDITOR_CONSTANTS
	ZOO_CONFIG_USE_DOUBLE_FOR_MS
	_CRT_SECURE_NO_WARNINGS
	_CRT_NONSTDC_NO_WARNINGS
)
target_compile_options(zzt_core PRIVATE /W3 "/FI${CMAKE_CURRENT_LIST_DIR}/win_compat.h")

add_executable(zzt_windows WIN32
	win32_frontend.c
	win32_io.c
	sharp_bilinear.c
)
target_include_directories(zzt_windows PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(zzt_windows PRIVATE c_std_99)
target_compile_definitions(zzt_windows PRIVATE
	_CRT_SECURE_NO_WARNINGS
	_CRT_NONSTDC_NO_WARNINGS
	WIN32_LEAN_AND_MEAN
	NOMINMAX
)
target_compile_options(zzt_windows PRIVATE /W3)
target_link_libraries(zzt_windows PRIVATE zzt_core user32 gdi32 winmm shell32)
set_target_properties(zzt_windows PROPERTIES OUTPUT_NAME "ZZT")

file(GLOB ZZT_RUNTIME_DATA CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_LIST_DIR}/../app/src/main/assets/*.ZZT"
	"${CMAKE_CURRENT_LIST_DIR}/../app/src/main/assets/*.HLP"
)
foreach(runtime_file IN LISTS ZZT_RUNTIME_DATA)
	add_custom_command(TARGET zzt_windows POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"${runtime_file}" "$<TARGET_FILE_DIR:zzt_windows>"
		VERBATIM
	)
endforeach()

enable_testing()
add_executable(zzt_windows_smoke win32_smoke.c win32_io.c)
target_include_directories(zzt_windows_smoke PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(zzt_windows_smoke PRIVATE c_std_99)
target_compile_definitions(zzt_windows_smoke PRIVATE
	_CRT_SECURE_NO_WARNINGS
	_CRT_NONSTDC_NO_WARNINGS
	WIN32_LEAN_AND_MEAN
	NOMINMAX
)
target_compile_options(zzt_windows_smoke PRIVATE /W3)
target_link_libraries(zzt_windows_smoke PRIVATE zzt_core)
add_test(NAME win32_engine_io_and_80x25
	COMMAND zzt_windows_smoke
		"${CMAKE_CURRENT_LIST_DIR}/../app/src/main/assets/DEMO.ZZT"
		"${CMAKE_CURRENT_BINARY_DIR}/demo-roundtrip-a.SAV"
		"${CMAKE_CURRENT_BINARY_DIR}/demo-roundtrip-b.SAV"
)

add_executable(zzt_renderer_test sharp_bilinear.c sharp_bilinear_test.c)
target_compile_features(zzt_renderer_test PRIVATE c_std_99)
add_test(NAME sharp_bilinear_integer_and_fractional
	COMMAND zzt_renderer_test
)

add_executable(zzt_sound_pcm_test sound_pcm_test.c)
target_compile_features(zzt_sound_pcm_test PRIVATE c_std_99)
target_link_libraries(zzt_sound_pcm_test PRIVATE zzt_core)
add_test(NAME win32_pcm_pascal_drum_timing
	COMMAND zzt_sound_pcm_test
)
