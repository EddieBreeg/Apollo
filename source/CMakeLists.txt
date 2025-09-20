AddLibrary(${PROJECT_NAME}Runtime
	TYPE SHARED
	EXPORT_MACRO BRK_API
	PROPERTIES ${COMMON_PROPERTIES}
	OPTIONS PRIVATE ${COMPILER_ARGS}
	DEFINITIONS PUBLIC $<$<NOT:$<CONFIG:Release>>:BRK_DEV=1>
		BRK_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
		BRK_VERSION_MINOR=${PROJECT_VERSION_MINOR}
		BRK_VERSION_PATCH=${PROJECT_VERSION_PATCH}
	INCLUDES PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	LINK PUBLIC spdlog::spdlog SDL3::SDL3
)

target_precompile_headers(${PROJECT_NAME}Runtime PRIVATE PCH.hpp)

add_subdirectory(core)
add_subdirectory(entry)
add_subdirectory(rendering)
add_subdirectory(ecs)
add_subdirectory(systems)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}Runtime ${PROJECT_NAME}Entry)


add_library(${PROJECT_NAME}::Runtime ALIAS ${PROJECT_NAME}Runtime)
add_library(${PROJECT_NAME}::Entry ALIAS ${PROJECT_NAME}Entry)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})