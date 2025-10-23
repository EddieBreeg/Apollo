include(Shaders)

option(APOLLO_EXAMPLE_MULTITHREADED "Whether to allow multiple threads to rasterize glyphs" ON)

AddShader(TextVertShader
	vertex "assets/text.vert.hlsl" ".brk/_assetCache/text.vert"
	DEBUG
)
AddShader(TextFragShader
	fragment "assets/text.frag.hlsl" ".brk/_assetCache/text.frag"
	DEBUG
)

AddExecutable(${PROJECT_NAME}Example SOURCES
	main.cpp
LINK PRIVATE ${PROJECT_NAME}::Editor SDL3::SDL3 freetype msdfgen-core
OPTIONS PRIVATE ${COMPILER_ARGS}
PROPERTIES ${COMMON_PROPERTIES}
)

if(${APOLLO_EXAMPLE_MULTITHREADED})
	target_compile_definitions(${PROJECT_NAME}Example PRIVATE USE_MULTITHREADING=1)
endif(${APOLLO_EXAMPLE_MULTITHREADED})

# add_dependencies(${PROJECT_NAME}Example TextVertShader TextFragShader)