file(GLOB RENDERING_HEADERS *.h *.hpp)

target_sources(${PROJECT_NAME}Runtime PRIVATE
	Buffer.cpp
	Command.cpp
	Context.cpp
	Device.cpp
	Material.cpp
	Pipeline.cpp
	RenderPass.cpp
	ShaderInfo.cpp
	Texture.cpp
	text/AtlasGenerator.cpp
	text/FontAtlas.cpp
	text/BatchRenderer.cpp
	Shader.cpp
	VertexTypes.cpp
	${RENDERING_HEADERS}
)

set(SupportedBackends)

if(${PROJECT_NAME}_ENABLE_D3D12)
	list(APPEND SupportedBackends APOLLO_D3D12=1)
endif()
if(${PROJECT_NAME}_ENABLE_VULKAN)
	list(APPEND SupportedBackends APOLLO_VULKAN=1)
	target_link_libraries(${PROJECT_NAME}Runtime PRIVATE Vulkan::Headers)
endif()

if(NOT SupportedBackends)
	message(FATAL_ERROR "Unsupported rendering backend")
endif()

target_compile_definitions(${PROJECT_NAME}Runtime PUBLIC ${SupportedBackends})