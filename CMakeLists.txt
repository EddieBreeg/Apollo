cmake_minimum_required(VERSION 3.24)
project(Apollo VERSION 0.0.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(TargetMacros)
include(TestMacros)

option(${PROJECT_NAME}_BUILD_TESTS "Whether to build tests for the project" ON)
option(${PROJECT_NAME}_INSTALL "Whether to enable install for ${PROJECT_NAME} targets" ${IS_ROOT_PROJECT})

set(${PROJECT_NAME}_BACKEND Vulkan CACHE STRING "The rendering backend used by the engine runtime")
set_property(CACHE ${PROJECT_NAME}_BACKEND PROPERTY STRINGS "Vulkan;D3D12")

if(${MSVC})
	set(COMPILER_ARGS /W4 /wd4201)
else()
	set(COMPILER_ARGS -Wall -Wextra)
	if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		list(APPEND COMPILER_ARGS -Wno-missing-field-initializers -Wno-missing-designated-field-initializers)
	endif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
endif(${MSVC})

set(COMMON_PROPERTIES CXX_STANDARD 20 COMPILE_WARNING_AS_ERROR ON)

if(PROJECT_IS_TOP_LEVEL)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif(PROJECT_IS_TOP_LEVEL)

add_subdirectory(deps)
add_subdirectory(source)
add_subdirectory(example)

if(${${PROJECT_NAME}_BUILD_TESTS})
	list(APPEND CMAKE_MODULE_PATH "${CATCH2_DIR}/extras")
	include(CTest)
	enable_testing()
	add_subdirectory(tests)
endif(${${PROJECT_NAME}_BUILD_TESTS})

set(CMAKE_FOLDER ${PROJECT_NAME})
