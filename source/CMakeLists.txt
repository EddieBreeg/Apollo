AddLibrary(${PROJECT_NAME}Runtime
	TYPE SHARED
	EXPORT_MACRO APOLLO_API
	PROPERTIES ${COMMON_PROPERTIES}
	OPTIONS PRIVATE ${COMPILER_ARGS}
	DEFINITIONS PUBLIC
		$<$<NOT:$<CONFIG:Release>>:APOLLO_DEV=1>
		APOLLO_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
		APOLLO_VERSION_MINOR=${PROJECT_VERSION_MINOR}
		APOLLO_VERSION_PATCH=${PROJECT_VERSION_PATCH}
		GLM_FORCE_DEPTH_ZERO_TO_ONE=1
		OPTIMIZE_OFF=${OPTIMIZE_OFF_MACRO}
	PRIVATE
		APOLLO_CORE=1
	INCLUDES PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	LINK PUBLIC spdlog::spdlog imgui glm::glm EnTT::EnTT nlohmann_json::nlohmann_json
	PRIVATE SDL3::SDL3 freetype msdfgen::msdfgen-core
)

target_precompile_headers(${PROJECT_NAME}Runtime PRIVATE PCH.hpp)

add_subdirectory(core)
add_subdirectory(rendering)
add_subdirectory(asset)
add_subdirectory(ecs)
add_subdirectory(systems)

add_subdirectory(editor)
add_subdirectory(entry)

add_subdirectory(tools)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}Runtime ${PROJECT_NAME}Entry)

add_library(${PROJECT_NAME}Editor INTERFACE)
target_compile_features(${PROJECT_NAME}Editor INTERFACE cxx_std_20)
target_link_libraries(${PROJECT_NAME}Editor INTERFACE
	${PROJECT_NAME}Runtime
	${PROJECT_NAME}EditorRuntime
	${PROJECT_NAME}EditorEntry
)

add_library(${PROJECT_NAME}::Runtime ALIAS ${PROJECT_NAME}Runtime)
add_library(${PROJECT_NAME}::Entry ALIAS ${PROJECT_NAME}Entry)
add_library(${PROJECT_NAME}::EditorEntry ALIAS ${PROJECT_NAME}EditorEntry)
add_library(${PROJECT_NAME}::EditorRuntime ALIAS ${PROJECT_NAME}Editor)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
add_library(${PROJECT_NAME}::Editor ALIAS ${PROJECT_NAME}Editor)