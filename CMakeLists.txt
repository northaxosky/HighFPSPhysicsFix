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

# ---- Project ----

project(
	HighFPSPhysicsFix
	VERSION 0.8.13
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
find_path(CLIBUTIL_INCLUDE_DIRS "CLibUtil/string.hpp")

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
		${CLIBUTIL_INCLUDE_DIRS}
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
			#/sdl             # Enable Additional Security Checks
			/utf-8           # Set Source and Executable character sets to UTF-8
			/Zi              # Debug Information Format

			/permissive-     # Standards conformance
			/Zc:preprocessor # Enable preprocessor conformance mode

			/wd4200          # nonstandard extension used : zero-sized array in struct/union

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
	)

	target_link_options(
		${PROJECT_NAME}
		PRIVATE
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF>"
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
