AddExecutable(ULIDGenerator SOURCES ULIDGen.cpp
	LINK PRIVATE ${PROJECT_NAME}Runtime
	OPTIONS PRIVATE ${COMPILER_ARGS}
	PROPERTIES ${COMMON_PROPERTIES}
)

AddExecutable(AtlasGen SOURCES AtlasGen.cpp
	LINK PRIVATE ${PROJECT_NAME}Runtime stb_image_write freetype msdfgen::msdfgen-core nlohmann_json::nlohmann_json
	OPTIONS PRIVATE ${COMPILER_ARGS}
	PROPERTIES ${COMMON_PROPERTIES}
)

set(_BIN_DIR $<TARGET_FILE_DIR:ULIDGenerator>)
add_custom_target(CopySlang ${CMAKE_COMMAND} -E copy $<TARGET_FILE:slang::slang> ${_BIN_DIR})

find_package(slang REQUIRED)
AddLibrary(ShaderCompiler TYPE STATIC 
SOURCES
	ShaderCompiler.cpp
	ShaderCompiler.hpp

OPTIONS PRIVATE ${COMPILER_ARGS}
LINK PUBLIC slang::slang PRIVATE spdlog::spdlog
PROPERTIES ${COMMON_PROPERTIES}
DEFINITIONS PRIVATE
	$<$<NOT:$<CONFIG:Release>>:APOLLO_DEV=1>
)
	
AddExecutable(shaderc SOURCES shaderc.cpp
OPTIONS PRIVATE ${COMPILER_ARGS}
PROPERTIES ${COMMON_PROPERTIES}
DEFINITIONS PRIVATE
	$<$<NOT:$<CONFIG:Release>>:APOLLO_DEV=1>
LINK PRIVATE ShaderCompiler
)

add_library(${PROJECT_NAME}::ShaderCompiler ALIAS ShaderCompiler)