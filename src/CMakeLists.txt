
find_package(PkgConfig REQUIRED)

if(LINK_TO_LIBGL)
	find_package(OpenGL REQUIRED)
	add_definitions(-DLINK_TO_LIBGL)
endif()

pkg_check_modules(SDL2 sdl2 REQUIRED)
pkg_check_modules(SDL2_TTF SDL2_ttf REQUIRED)
pkg_check_modules(ZLIB zlib REQUIRED)
pkg_check_modules(PNG libpng>=1.5 REQUIRED)

if(NOT NO_AUDIO)
	pkg_check_modules(SDL2_MIXER SDL2_mixer REQUIRED)
endif()

set(SRCs
	main.c
	cli.c
	log.c
	util.c
	taiseigl.c
	random.c
	config.c
	color.c
	gamepad.c
	stage.c
	stagedraw.c
	stagetext.c
	replay.c
	global.c
	events.c
	player.c
	projectile.c
	progress.c
	enemy.c
	item.c
	list.c
	hashtable.c
	boss.c
	plrmodes.c
	laser.c
	dialog.c
	fbo.c
	vbo.c
	stageutils.c
	matrix.c
	video.c
	transition.c
	color.c
	difficulty.c
	audio_common.c
	menu/menu.c
	menu/mainmenu.c
	menu/options.c
	menu/stageselect.c
	menu/replayview.c
	menu/ingamemenu.c
	menu/gameovermenu.c
	menu/savereplay.c
	menu/difficultyselect.c
	menu/charselect.c
	menu/spellpractice.c
	menu/common.c
	stages/stage1.c
	stages/stage1_events.c
	stages/stage2.c
	stages/stage2_events.c
	stages/stage3.c
	stages/stage3_events.c
	stages/stage4.c
	stages/stage4_events.c
	stages/stage5.c
	stages/stage5_events.c
	stages/stage6.c
	stages/stage6_events.c
	ending.c
	credits.c
	resource/resource.c
	resource/texture.c
	resource/animation.c
	resource/font.c
	resource/shader.c
	resource/model.c
	resource/postprocess.c
	rwops/rwops_zlib.c
	rwops/rwops_segment.c
	rwops/rwops_autobuf.c
)

if(USE_SDL2_PATHS)
    set(SRCs ${SRCs} paths/sdl.c)
elseif(RELATIVE)
	set(SRCs ${SRCs} paths/relative.c)
else()
	set(SRCs ${SRCs} paths/static.c)
endif()

if(NO_AUDIO)
	set(SRCs ${SRCs}
		audio_null.c
		resource/bgm_null.c
		resource/sfx_null.c
	)
else()
	set(SRCs ${SRCs}
		audio_mixer.c
		resource/bgm_mixer.c
		resource/sfx_mixer.c
	)
endif()

if(DEFINED TAISEI_DEBUG)
	if(TAISEI_DEBUG)
		set(CMAKE_BUILD_TYPE "Debug")
	else()
		set(CMAKE_BUILD_TYPE "Release")
	endif()
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release")
	#message(WARNING "CMAKE_BUILD_TYPE was not set, assuming ${CMAKE_BUILD_TYPE}.")
endif()

message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

set(DEBUG_FLAGS "-ggdb -fno-omit-frame-pointer")

if(DEBUG_USE_UBSAN)
	set(DEBUG_FLAGS "${DEBUG_FLAGS} -fsanitize=undefined")
endif()

if(DEBUG_USE_ASAN)
	set(DEBUG_FLAGS "${DEBUG_FLAGS} -fsanitize=address")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG ${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${DEBUG_FLAGS}")

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}" -Wall -Wno-parentheses -Wtype-limits -std=c11 -pedantic)

if(RELATIVE)
    add_definitions(-DRELATIVE)
endif()

if(WERROR)
	add_definitions(-Werror)
endif()

if(FATALERRS)
	add_definitions(-Wfatal-errors)
endif()

if(WIN32 OR APPLE)
	add_definitions(-DLOG_FATAL_MSGBOX)
endif()

set(LIBs ${LIBs}
	${SDL2_LIBRARIES}
	${SDL2_TTF_LIBRARIES}
	${PNG_LIBRARIES}
	${ZLIB_LIBRARIES}
	${OPENGL_gl_LIBRARY}
	m)

set(LIBDIRs
	${SDL2_LIBRARY_DIRS}
	${SDL2_TTF_LIBRARY_DIRS}
	${PNG_LIBRARY_DIRS}
	${ZLIB_LIBRARY_DIRS}
	)

if(WIN32)
	set(LIBs ${LIBs} -ldxguid -lwinmm)

	string(REPLACE "gcc" "windres" CMAKE_RC_COMPILER_INIT ${CMAKE_C_COMPILER})
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> <SOURCE>")

	set(SRCs ${SRCs} taisei.rc)
endif()

set(INCs
	${CMAKE_CURRENT_SOURCE_DIR}
	${SDL2_INCLUDE_DIRS}
	${SDL2_TTF_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${PNG_INCLUDE_DIRS}
	)

# stupid cmake caching SHIT
if(NOT NO_AUDIO)
	set(INCs ${INCs} ${SDL2_MIXER_INCLUDE_DIRS})
	set(LIBs ${LIBs} ${SDL2_MIXER_LIBRARIES})
	set(LIBDIRs ${LIBDIRs} ${SDL2_MIXER_LIBRARY_DIRS})
endif()

set(CMAKE_REQUIRED_INCLUDES ${INCs})
include_directories(${INCs})

check_symbol_exists(_POSIX_VERSION "unistd.h" POSIX)

if(POSIX)
	add_definitions(-D__POSIX__)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)
	if(HAVE_BACKTRACE)
		add_definitions(-DLOG_ENABLE_BACKTRACE)
	endif()
endif()

if (CMAKE_GENERATOR STREQUAL "Ninja" AND
    ((CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.9) OR
     (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 3.5)))
    # Force colored warnings in Ninja's output, if the compiler has -fdiagnostics-color support.
    # Rationale in https://github.com/ninja-build/ninja/issues/814
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")
endif()

list(SORT SRCs)
list(REMOVE_DUPLICATES SRCs)

link_directories(${LIBDIRs})

if(TAISEI_WIN32_CONSOLE)
	list(REMOVE_ITEM LIBs -mwindows)
	add_executable(taisei MACOSX_BUNDLE ${SRCs})
else()
	add_executable(taisei MACOSX_BUNDLE WIN32 ${SRCs})
endif()

target_link_libraries(taisei ${LIBs})

set(MACOSX_BUNDLE_BUNDLE_NAME "Taisei")

if(APPLE)
	set_target_properties(taisei PROPERTIES
		OUTPUT_NAME "Taisei"
		MACOSX_BUNDLE_ICON_FILE "Taisei"
		MACOSX_BUNDLE_BUNDLE_NAME "Taisei"
	)

	install(TARGETS taisei BUNDLE DESTINATION .)
	set(OSX_INSTALLSCRIPT_PATH "${PROJECT_SOURCE_DIR}/scripts/OSXInstallDylibs.cmake")
	configure_file("${OSX_INSTALLSCRIPT_PATH}.in" "${OSX_INSTALLSCRIPT_PATH}.out" @ONLY)
	install(SCRIPT "${OSX_INSTALLSCRIPT_PATH}.out")
elseif(RELATIVE)
	install(TARGETS taisei RUNTIME DESTINATION .)
else()
	install(TARGETS taisei RUNTIME DESTINATION bin)
endif()
