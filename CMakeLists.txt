# SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)
project(YaST VERSION 24.0.106 LANGUAGES C)

# Default build type for single-config generators only.
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif ()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Bundle runtime assets (layouts, flags, icons) into the executable via GResource
pkg_get_variable(GLIB_COMPILE_RESOURCES gio-2.0 glib_compile_resources)
if (NOT GLIB_COMPILE_RESOURCES)
	find_program(GLIB_COMPILE_RESOURCES glib-compile-resources REQUIRED)
endif ()

set(RESOURCE_PREFIX "/com/philarmstead/yast")
set(GRESOURCE_XML "${CMAKE_CURRENT_BINARY_DIR}/resources.gresource.xml")
set(GRESOURCE_C "${CMAKE_CURRENT_BINARY_DIR}/resources.c")

file(GLOB LAYOUT_FILES CONFIGURE_DEPENDS RELATIVE "${REPO_ROOT}"
	"${REPO_ROOT}/layouts/*.ui"
	"${REPO_ROOT}/layouts/*.css"
)
file(GLOB ASSET_ICONS CONFIGURE_DEPENDS RELATIVE "${REPO_ROOT}"
	"${REPO_ROOT}/assets/icons/*"
)
file(GLOB ASSET_FLAGS CONFIGURE_DEPENDS RELATIVE "${REPO_ROOT}"
	"${REPO_ROOT}/assets/flags/*.png"
)
set(RESOURCE_FILES ${LAYOUT_FILES} ${ASSET_ICONS} ${ASSET_FLAGS})

set(GRESOURCE_ENTRIES "")
foreach (res_file IN LISTS RESOURCE_FILES)
	# Text assets compress well; PNG flags are already compressed.
	set(compress "")
	if (res_file MATCHES "\\.(ui|css|svg)$")
		set(compress " compressed=\"true\"")
	endif ()
	string(APPEND GRESOURCE_ENTRIES "    <file${compress}>${res_file}</file>\n")
endforeach ()

file(WRITE "${GRESOURCE_XML}"
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<gresources>
  <gresource prefix=\"${RESOURCE_PREFIX}\">
${GRESOURCE_ENTRIES}  </gresource>
</gresources>
")

add_custom_command(
	OUTPUT "${GRESOURCE_C}"
	COMMAND ${GLIB_COMPILE_RESOURCES}
	--target=${GRESOURCE_C}
	--sourcedir=${REPO_ROOT}
	--generate-source
	${GRESOURCE_XML}
	DEPENDS ${GRESOURCE_XML} ${RESOURCE_FILES}
	WORKING_DIRECTORY ${REPO_ROOT}
	COMMENT "Compiling bundled resources"
	VERBATIM
)

add_executable(yast
	${GRESOURCE_C}
	src/main.c
	src/app/callbacks.c
	src/app/cache.c
	src/app/options.c
	src/app/player-table.c
	src/app/player.c
	src/app/search.c
	src/app/search-handler.c
	src/app/ui.c
	src/app/callbacks/best-eleven.c
	src/app/callbacks/club-search.c
	src/app/callbacks/filters.c
	src/app/callbacks/player.c
	src/app/helpers/date.c
	src/app/helpers/formatter.c
	src/app/helpers/game.c
	src/app/helpers/vector-shared-pointer.c
	src/app/ui/best-eleven.c
	src/app/ui/player-info.c
	src/core/logger.c
	src/platform/linux.c
	src/platform/macos.c
	src/platform/windows.c
)

# Prevent a console/terminal window in Release builds.
if (WIN32)
	# Windows: link as a GUI subsystem app so no console is allocated.
	target_link_options(yast PRIVATE $<$<CONFIG:Release>:-mwindows>)
elseif (APPLE)
	# macOS: build a .app bundle in Release so launching does not open a Terminal.
	set_target_properties(yast PROPERTIES MACOSX_BUNDLE $<$<CONFIG:Release>:TRUE>)
endif ()

# Allow including headers using absolute paths relative to the project `src/` directory,
# e.g. #include "platform/platform.h"
target_include_directories(yast PRIVATE
	${PROJECT_SOURCE_DIR}/src
)

# Use the project name for the public executable while keeping a lowercase CMake target.
set_target_properties(yast PROPERTIES
	OUTPUT_NAME "${PROJECT_NAME}"
)

target_compile_features(yast PRIVATE c_std_99)

target_compile_options(yast PRIVATE
	$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:
	-Wall
	-Wextra
	-Wpedantic
	-Wshadow
	-Wconversion
	-Wsign-conversion
	-Wstrict-prototypes
	-Wmissing-prototypes
	-Wno-missing-field-initializers
	>
	$<$<C_COMPILER_ID:MSVC>:
	/W4
	>
)

target_compile_definitions(yast PRIVATE
	$<$<PLATFORM_ID:Windows>:ARCH_WIN>
	$<$<PLATFORM_ID:Linux>:ARCH_LINUX>
	$<$<PLATFORM_ID:Darwin>:ARCH_MACOS>
	$<$<CONFIG:Debug>:DEBUG>
	APP_NAME="${PROJECT_NAME}"
	APP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
	APP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
	APP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
	GNU_SOURCE
)

target_link_libraries(yast PRIVATE PkgConfig::GTK4)

# Export runtime symbols on Linux so GtkBuilder can find G_MODULE_EXPORT callbacks
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# -rdynamic makes all symbols available to the dynamic linker for dlsym/lookup
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
endif ()

add_custom_target(info
	COMMAND ${CMAKE_COMMAND} -E echo "Generator: ${CMAKE_GENERATOR}"
	COMMAND ${CMAKE_COMMAND} -E echo "Config: $<CONFIG>"
	COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
	COMMAND ${CMAKE_COMMAND} -E echo "Target: $<TARGET_FILE:yast>"
)

add_custom_target(run
	COMMAND $<TARGET_FILE:yast>
	DEPENDS yast
	WORKING_DIRECTORY $<TARGET_FILE_DIR:yast>
	USES_TERMINAL
)