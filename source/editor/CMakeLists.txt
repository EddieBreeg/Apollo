file(GLOB_RECURSE EDITOR_HEADERS *.h *.hpp)

AddLibrary(${PROJECT_NAME}EditorRuntime
SOURCES
	asset/LoadMaterial.cpp
	asset/LoadMetadata.cpp
	asset/LoadShader.cpp
	asset/LoadTexture.cpp
	Editor.cpp
	${EDITOR_HEADERS}
PROPERTIES ${COMMON_PROPERTIES}
OPTIONS PRIVATE ${COMPILER_ARGS}
DEFINITIONS PUBLIC BRK_EDITOR=1
LINK PRIVATE ${PROJECT_NAME}Runtime stb_image SDL3::SDL3
)

if(${PROJECT_NAME}_BACKEND STREQUAL "Vulkan")
	target_link_libraries(${PROJECT_NAME}EditorRuntime PRIVATE
		spirv-reflect-static
	)
endif(${PROJECT_NAME}_BACKEND STREQUAL "Vulkan")
