file(GLOB RENDERING_HEADERS *.h *.hpp)

target_sources(${PROJECT_NAME}Runtime PRIVATE
	Buffer.cpp
	Context.cpp
	Device.cpp
	Material.cpp
	Pipeline.cpp
	Texture.cpp
	text/AtlasGenerator.cpp
	text/FontAtlas.cpp
	text/BatchRenderer.cpp
	Shader.cpp
	VertexTypes.cpp
	${RENDERING_HEADERS}
)

if(${PROJECT_NAME}_BACKEND STREQUAL "D3D12")
	set(BACKEND_MACRO_NAME APOLLO_D3D12)
elseif(${PROJECT_NAME}_BACKEND STREQUAL "Vulkan")
	set(BACKEND_MACRO_NAME APOLLO_VULKAN)
	target_link_libraries(${PROJECT_NAME}Runtime PRIVATE glslang::glslang glslang::SPIRV glslang::glslang-default-resource-limits spirv-reflect-static)
else()
	message(FATAL_ERROR "Unsupported rendering backend")
endif(${PROJECT_NAME}_BACKEND STREQUAL "D3D12")

target_compile_definitions(${PROJECT_NAME}Runtime PUBLIC ${BACKEND_MACRO_NAME}=1)