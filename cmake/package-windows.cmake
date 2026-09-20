# SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Assembles a redistributable Windows directory next to the executable.
#
# A MinGW build of YaST links the GTK4 stack dynamically, so the bare .exe only runs on a machine
# that already has an MSYS2 installation on PATH. Everything GTK resolves at runtime has to travel
# with it: the transitive DLL closure, the gdk-pixbuf loader modules, the compiled GSettings
# schemas (GTK aborts on startup without them) and the Adwaita icon theme.
#
# Each of those trees is pruned to what this application can actually reach, because copying them
# whole costs several times the size of the parts GTK ever loads. Every prune is keyed off a
# property of the app rather than a hard-coded file list, so it stays correct as GTK moves on.
#
# Run in script mode with -D EXE, DEST, GTK_PREFIX, OBJDUMP and GLIB_COMPILE_SCHEMAS set.

cmake_minimum_required(VERSION 3.20)

foreach (required IN ITEMS EXE DEST GTK_PREFIX OBJDUMP)
	if (NOT ${required})
		message(FATAL_ERROR "package-windows.cmake requires -D ${required}=...")
	endif ()
endforeach ()

file(REMOVE_RECURSE "${DEST}")
file(MAKE_DIRECTORY "${DEST}")

# GLib, gdk-pixbuf and GTK are relocatable on Windows: each locates its data by asking where its
# own DLL lives, via g_win32_get_package_installation_directory_of_module(). That helper treats a
# "bin" directory as a marker and walks up to its parent, so placing the binaries in bin/ with
# lib/ and share/ as siblings of it is the layout the MSYS2 builds already expect. It also keeps
# the installed application directory tidy, since the DLLs sit one level below the shortcut target.
set(BIN_DEST "${DEST}/bin")

# GET_RUNTIME_DEPENDENCIES reads it from this variable, which script mode does not populate.
set(CMAKE_OBJDUMP "${OBJDUMP}")

# ----------------------------------------------------------------------------------------------
# gdk-pixbuf loaders
#
# gdk-pixbuf compiles the common raster formats into its own library and ships the rest as
# dlopen()ed modules. YaST only decodes the PNG flags and the two SVGs referenced from the
# stylesheet, so every other module is dead weight — and an expensive kind, because each one drags
# its whole codec stack (libtiff, libwebp, libjxl, libavif, libheif and their dependencies) into
# the DLL closure resolved below.
# ----------------------------------------------------------------------------------------------

set(WANTED_LOADERS "svg" "png")

file(GLOB PIXBUF_MODULE_DIRS "${GTK_PREFIX}/lib/gdk-pixbuf-2.0/*")
set(KEPT_LOADERS "")
foreach (dir IN LISTS PIXBUF_MODULE_DIRS)
	file(GLOB loaders "${dir}/loaders/*.dll")
	foreach (loader IN LISTS loaders)
		get_filename_component(loader_name "${loader}" NAME)
		foreach (wanted IN LISTS WANTED_LOADERS)
			if (loader_name MATCHES "pixbufloader[-_]${wanted}\\.dll$")
				list(APPEND KEPT_LOADERS "${loader}")
			endif ()
		endforeach ()
	endforeach ()
endforeach ()

# ----------------------------------------------------------------------------------------------
# DLL closure
#
# Resolved from the PE import tables rather than a maintained list, so a new GTK dependency is
# picked up automatically. The loaders are dlopen()ed and never linked, so they are declared as
# MODULES: their own dependencies join the closure, while the modules themselves are copied into
# the layout gdk-pixbuf expects further down.
# ----------------------------------------------------------------------------------------------

file(GET_RUNTIME_DEPENDENCIES
	EXECUTABLES "${EXE}"
	MODULES ${KEPT_LOADERS}
	RESOLVED_DEPENDENCIES_VAR RESOLVED
	UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED
	DIRECTORIES "${GTK_PREFIX}/bin"
	PRE_EXCLUDE_REGEXES "^api-ms-win-.*" "^ext-ms-.*"
	POST_EXCLUDE_REGEXES "[Ss]ystem32" "[Ww]indows[/\\\\][Ss]ys"
)

foreach (dll IN LISTS RESOLVED)
	file(COPY "${dll}" DESTINATION "${BIN_DEST}")
endforeach ()
list(LENGTH RESOLVED DLL_COUNT)

# Anything left unresolved that is not a Windows system DLL is a missing runtime dependency, and
# the failure only shows up on the end user's machine. Surface it here instead.
foreach (dll IN LISTS UNRESOLVED)
	message(WARNING "Unresolved dependency, the bundle may not run elsewhere: ${dll}")
endforeach ()

file(COPY "${EXE}" DESTINATION "${BIN_DEST}")

# The loaders and their cache keep their original directory nesting, because gdk-pixbuf derives
# the module search path from the cache's own location.
#
# The cache is filtered rather than regenerated: MSYS2 builds gdk-pixbuf relocatable, so the
# shipped cache holds paths that resolve against the module directory, whereas re-running
# gdk-pixbuf-query-loaders here would bake in this machine's absolute paths and break everywhere
# else. Entries are blocks introduced by a quoted module path, so dropping the blocks whose module
# was pruned is a purely textual edit.
foreach (dir IN LISTS PIXBUF_MODULE_DIRS)
	get_filename_component(version_dir "${dir}" NAME)
	set(module_dest "${DEST}/lib/gdk-pixbuf-2.0/${version_dir}")

	foreach (loader IN LISTS KEPT_LOADERS)
		if (loader MATCHES "/${version_dir}/loaders/")
			file(COPY "${loader}" DESTINATION "${module_dest}/loaders")
		endif ()
	endforeach ()

	if (NOT EXISTS "${dir}/loaders.cache")
		continue ()
	endif ()

	file(STRINGS "${dir}/loaders.cache" cache_lines)
	set(filtered "")
	set(keep_block TRUE)
	foreach (line IN LISTS cache_lines)
		if (line MATCHES "^\"(.*\\.dll)\"$")
			# A quoted module path opens a new block; keep it only if that module survived.
			set(module_path "${CMAKE_MATCH_1}")
			set(keep_block FALSE)
			foreach (loader IN LISTS KEPT_LOADERS)
				get_filename_component(loader_name "${loader}" NAME)
				if (module_path MATCHES "${loader_name}$")
					set(keep_block TRUE)
				endif ()
			endforeach ()
		endif ()
		if (keep_block)
			string(APPEND filtered "${line}\n")
		endif ()
	endforeach ()
	file(WRITE "${module_dest}/loaders.cache" "${filtered}")
endforeach ()

# ----------------------------------------------------------------------------------------------
# Icon theme
#
# GTK4 embeds only its "image-missing" fallback, so Adwaita has to ship or the search entry's
# clear button, the dropdown arrows and the spin button steppers all render blank.
#
# YaST draws its own iconography with Cairo paths and requests no icon by name, so the only icons
# reachable at runtime are the symbolic ones GTK's own widgets ask for. Adwaita keeps those in a
# single "symbolic" tree, separate from the full-colour and legacy raster trees that make up the
# bulk of the theme, which makes that one directory the entire useful payload. index.theme entries
# pointing at the trees left behind are simply skipped by the loader.
# ----------------------------------------------------------------------------------------------

set(ADWAITA "${GTK_PREFIX}/share/icons/Adwaita")
if (IS_DIRECTORY "${ADWAITA}/symbolic")
	file(COPY "${ADWAITA}/symbolic" DESTINATION "${DEST}/share/icons/Adwaita")
	file(COPY "${ADWAITA}/index.theme" DESTINATION "${DEST}/share/icons/Adwaita")
elseif (IS_DIRECTORY "${ADWAITA}")
	# An older layout with no separate symbolic tree; ship it whole rather than guess.
	message(STATUS "Adwaita has no symbolic tree, bundling the full theme")
	file(COPY "${ADWAITA}" DESTINATION "${DEST}/share/icons")
else ()
	message(WARNING
		"Adwaita icon theme not found in ${GTK_PREFIX}/share/icons, GTK's widget icons will be "
		"missing. Install mingw-w64-<env>-adwaita-icon-theme."
	)
endif ()

# hicolor is the fallback theme every icon lookup ends at. Only its index.theme matters; the
# directories beneath it are empty by design.
if (EXISTS "${GTK_PREFIX}/share/icons/hicolor/index.theme")
	file(COPY "${GTK_PREFIX}/share/icons/hicolor/index.theme"
		DESTINATION "${DEST}/share/icons/hicolor"
	)
endif ()

# ----------------------------------------------------------------------------------------------
# GSettings schemas
#
# Only the compiled blob is read at runtime; the .xml sources are build-time inputs.
# ----------------------------------------------------------------------------------------------

set(SCHEMA_SRC "${GTK_PREFIX}/share/glib-2.0/schemas")
set(SCHEMA_DEST "${DEST}/share/glib-2.0/schemas")
if (EXISTS "${SCHEMA_SRC}/gschemas.compiled")
	file(COPY "${SCHEMA_SRC}/gschemas.compiled" DESTINATION "${SCHEMA_DEST}")
elseif (GLIB_COMPILE_SCHEMAS)
	file(COPY "${SCHEMA_SRC}/" DESTINATION "${SCHEMA_DEST}"
		FILES_MATCHING PATTERN "*.xml" PATTERN "*.gschema.override"
	)
	execute_process(COMMAND "${GLIB_COMPILE_SCHEMAS}" "${SCHEMA_DEST}" COMMAND_ERROR_IS_FATAL ANY)
	file(GLOB schema_sources "${SCHEMA_DEST}/*.xml" "${SCHEMA_DEST}/*.gschema.override")
	file(REMOVE ${schema_sources})
else ()
	message(FATAL_ERROR
		"No gschemas.compiled in ${SCHEMA_SRC} and glib-compile-schemas was not found. GTK will "
		"abort at startup without it."
	)
endif ()

# ----------------------------------------------------------------------------------------------
# Size report
#
# The bundle is dominated by a handful of DLLs, so name the worst offenders. It makes an
# unexpected jump in distribution size traceable to the dependency that caused it.
# ----------------------------------------------------------------------------------------------

file(GLOB_RECURSE BUNDLED_FILES "${DEST}/*")
set(TOTAL_BYTES 0)
set(SIZED_BINARIES "")
foreach (entry IN LISTS BUNDLED_FILES)
	file(SIZE "${entry}" entry_bytes)
	math(EXPR TOTAL_BYTES "${TOTAL_BYTES} + ${entry_bytes}")
	if (entry MATCHES "\\.(dll|exe)$")
		# Left-pad to a fixed width so a plain lexicographic sort orders the list by size.
		string(LENGTH "${entry_bytes}" digits)
		math(EXPR pad_width "12 - ${digits}")
		string(REPEAT "0" ${pad_width} padding)
		get_filename_component(entry_name "${entry}" NAME)
		list(APPEND SIZED_BINARIES "${padding}${entry_bytes} ${entry_name}")
	endif ()
endforeach ()

list(SORT SIZED_BINARIES ORDER DESCENDING)
math(EXPR TOTAL_MIB "${TOTAL_BYTES} / 1048576")
message(STATUS "Bundled ${DLL_COUNT} DLLs, ${TOTAL_MIB} MiB total, into ${DEST}")

list(LENGTH SIZED_BINARIES BINARY_COUNT)
math(EXPR REPORT_LAST "${BINARY_COUNT} - 1")
if (REPORT_LAST GREATER 9)
	set(REPORT_LAST 9)
endif ()
foreach (index RANGE ${REPORT_LAST})
	list(GET SIZED_BINARIES ${index} entry)
	string(REGEX MATCH "^0*([0-9]+) (.+)$" _ "${entry}")
	math(EXPR entry_kib "${CMAKE_MATCH_1} / 1024")
	message(STATUS "  ${entry_kib} KiB  ${CMAKE_MATCH_2}")
endforeach ()
