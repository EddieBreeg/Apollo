file(GLOB RENDERING_HEADERS *.h *.hpp)

target_sources(${PROJECT_NAME}Runtime PRIVATE
	Buffer.cpp
	Device.cpp
	Material.cpp
	Pipeline.cpp
	Renderer.cpp
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
else()
	message(FATAL_ERROR "Unsupported rendering backend")
endif(${PROJECT_NAME}_BACKEND STREQUAL "D3D12")

target_compile_definitions(${PROJECT_NAME}Runtime PUBLIC ${BACKEND_MACRO_NAME}=1)