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