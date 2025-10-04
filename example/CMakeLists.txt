include(Shaders)

AddShader(TexturedQuadVShader
	vertex "assets/texturedQuad.vert.hlsl" ".brk/_assetCache/texturedQuad.vert"
	DEBUG
)
AddShader(TexturedQuadFShader
	fragment "assets/texturedQuad.frag.hlsl" ".brk/_assetCache/texturedQuad.frag"
	DEBUG
)
AddShader(TextFragShader
	fragment "assets/text.frag.hlsl" ".brk/_assetCache/text.frag"
	DEBUG
)

AddExecutable(${PROJECT_NAME}Example SOURCES
	main.cpp Text.cpp
LINK PRIVATE ${PROJECT_NAME}::Editor SDL3::SDL3 freetype msdfgen-core
OPTIONS PRIVATE ${COMPILER_ARGS}
PROPERTIES ${COMMON_PROPERTIES}
)

add_dependencies(${PROJECT_NAME}Example TexturedQuadVShader TexturedQuadFShader TextFragShader)