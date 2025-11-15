file(GLOB_RECURSE EDITOR_HEADERS *.h *.hpp)

AddLibrary(${PROJECT_NAME}EditorRuntime
SOURCES
	asset/LoadFont.cpp
	asset/LoadMaterial.cpp
	asset/LoadMesh.cpp
	asset/LoadMetadata.cpp
	asset/LoadScene.cpp
	asset/LoadShader.cpp
	asset/LoadTexture.cpp
	asset/Manager.cpp
	Editor.cpp
	RegisterComponents.cpp
	${EDITOR_HEADERS}
PROPERTIES ${COMMON_PROPERTIES}
OPTIONS PRIVATE ${COMPILER_ARGS}
DEFINITIONS PUBLIC APOLLO_EDITOR=1
LINK PRIVATE ${PROJECT_NAME}Runtime stb_image SDL3::SDL3 freetype msdfgen::msdfgen-core assimp::assimp
)