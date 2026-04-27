cmake_minimum_required(VERSION 3.21)

# ---- Options ----

option(COPY_BUILD "Copy the build output to the Fallout 4 directory." TRUE)

# ---- Cache build vars ----

macro(set_from_environment VARIABLE)
	if (NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif ()
endmacro()

set_from_environment(Fallout4Path)
set_from_environment(VCPKG_ROOT)

if (DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
else ()
	message(
		WARNING
		"Variable VCPKG_ROOT is not set. Continuing without vcpkg."
	)
endif () 
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# ---- Project version (git-derived with tarball fallback) ----
#
# Default fallback version, used when:
#   - git is not available on PATH (tarball builds)
#   - the source tree is not a git checkout
#   - `git describe` fails (no tags reachable from HEAD)
set(FALLBACK_VERSION 0.8.13)

set(PROJECT_VERSION_FROM_GIT "${FALLBACK_VERSION}")
set(PROJECT_VERSION_DESCRIBE "${FALLBACK_VERSION}")

find_package(Git QUIET)
if (Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" describe --tags --always --dirty --match "[0-9]*"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		OUTPUT_VARIABLE   GIT_DESCRIBE
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
		RESULT_VARIABLE   GIT_DESCRIBE_RESULT
	)
	if (GIT_DESCRIBE_RESULT EQUAL 0 AND GIT_DESCRIBE)
		set(PROJECT_VERSION_DESCRIBE "${GIT_DESCRIBE}")
		# Extract a strict M.m.p prefix for project(VERSION ...) and the
		# F4SE PluginVersionData consumer. Anything past the third number
		# (commit count, sha, "-dirty") is preserved in DESCRIBE for logs
		# and the resource string.
		if (GIT_DESCRIBE MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
			set(PROJECT_VERSION_FROM_GIT "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
		endif ()
	endif ()
endif ()

message(STATUS "HighFPSPhysicsFix version: ${PROJECT_VERSION_FROM_GIT} (describe: ${PROJECT_VERSION_DESCRIBE})")

# ---- Project ----

project(
	HighFPSPhysicsFix
	VERSION ${PROJECT_VERSION_FROM_GIT}
	LANGUAGES CXX RC
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
        @ONLY
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY
)

file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/droidsans.font" DROIDSANS_FONT_FILE)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/resources.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/resources.rc
        @ONLY
)

# ---- Include guards ----

if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
		FATAL_ERROR
		"In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
	)
endif ()

# ---- Globals ----

add_compile_definitions(
	F4SE_SUPPORT_XBYAK
	COMMONLIB_RUNTIMECOUNT=3
)

if (MSVC)
	if (NOT ${CMAKE_GENERATOR} STREQUAL "Ninja")
		add_compile_options(
			/MP	# Build with Multiple Processes
		)
	endif ()
endif ()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(Boost_USE_STATIC_LIBS ON)

# ---- Dependencies ----

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibF4/include/F4SE/F4SE.h")
	message(
		FATAL_ERROR
		"CommonLibF4 submodule missing. Run: git submodule update --init --recursive"
	)
endif ()
# Upstream Dear-Modding-FO4/commonlibf4 removed its CMakeLists.txt (commit 6d240381)
# in favor of xmake. We vendor a CMake build wrapper into the submodule at configure
# time so the existing CMake build keeps working. The wrapper lives at
# cmake/CommonLibF4-CMakeLists.txt and is the same one used by FO4Upscaling.
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/CommonLibF4-CMakeLists.txt
	${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibF4/CMakeLists.txt
	COPYONLY
)
add_subdirectory(extern/CommonLibF4 CommonLibF4 EXCLUDE_FROM_ALL)

find_path(SRELL_INCLUDE_DIRS "srell.hpp")

find_package(directxtk CONFIG REQUIRED)
find_package(spdlog REQUIRED CONFIG)

# ---- Add source files ----

include(cmake/headerlist.cmake)
include(cmake/sourcelist.cmake)

source_group(
	TREE
		${CMAKE_CURRENT_SOURCE_DIR}
	FILES
		${headers}
		${sources}
)

source_group(
	TREE
		${CMAKE_CURRENT_BINARY_DIR}
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
)

# ---- Create DLL ----

add_library(
	${PROJECT_NAME}
	SHARED
	${headers}
        ${sources}
        ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/resources.rc
        .clang-format
)

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
		cxx_std_23
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		_UNICODE
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		CommonLibF4::CommonLibF4
		spdlog::spdlog
		Microsoft::DirectXTK
)

target_precompile_headers(
	${PROJECT_NAME}
	PRIVATE
		include/PCH.h
)

if (MSVC)
	target_compile_options(
		${PROJECT_NAME}
		PRIVATE
			/sdl                    # Additional security checks
			/W4                     # Warning level 4
			/WX                     # Treat warnings as errors
			/GS                     # Buffer security check
			/guard:cf               # Control Flow Guard
			# /Qspectre             # Deferred: requires VS "Spectre-mitigated libs"
			#                       # individual component; enable on CI runners
			#                       # that have it installed.
			/Brepro                 # Reproducible build (deterministic timestamps)
			/utf-8                  # Source/exec charset = UTF-8
			/Zi                     # Debug Information Format

			/permissive-            # Standards conformance
			/Zc:preprocessor        # Preprocessor conformance
			/Zc:__cplusplus         # Report correct __cplusplus value
			/Zc:throwingNew         # Assume operator new throws
			/Zc:inline              # Remove unreferenced COMDAT (Release default; safe to keep global)

			/wd4200                 # nonstandard extension: zero-sized array
			/wd4100                 # unreferenced parameter (Event(code,data)
			#                       # dispatcher callback signature requires both
			#                       # args even when handlers ignore them)
			# Pre-existing warnings deferred to Tier 4 (kept off /WX):
			#   (none after fixing C4996 deprecations and C4703 hMonitor init
			#   in this tier; /WX is enabled below).

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/JMC-;/Ob3;/GL;/Gw;/Gy>"
	)

	target_link_options(
		${PROJECT_NAME}
		PRIVATE
			/CETCOMPAT
			/DYNAMICBASE
			/NXCOMPAT
			/HIGHENTROPYVA
			/guard:cf
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/DEBUG:FULL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/DEBUG:FULL;/OPT:REF;/OPT:ICF;/LTCG>"
	)
endif ()

# ---- Post build ----

if (COPY_BUILD)
	if (DEFINED Fallout4Path)
		add_custom_command(
			TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${Fallout4Path}/Data/F4SE/Plugins/
		)
	else ()
		message(
			WARNING
			"Variable Fallout4Path is not defined. Skipping post-build copy command."
		)
	endif ()
endif ()

# ---- Build artifacts ----

set(SCRIPT "scripts/archive_artifacts.py")
set(TARGET_NAME "archive")

add_custom_target(
	create_artifact
	COMMAND
		"${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}"
		"--name=\"${PROJECT_NAME}\""
		"--dll=\"$<TARGET_FILE:${PROJECT_NAME}>\""
		"--out-dir=\"${TARGET_NAME}\""
		"--src-dir=\"${CMAKE_CURRENT_SOURCE_DIR}\""
	WORKING_DIRECTORY
		${CMAKE_CURRENT_BINARY_DIR}
	SOURCES
		${SCRIPT}
)

add_dependencies(
	create_artifact
	${PROJECT_NAME}
)

source_group(
	TREE ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${SCRIPT}
)
