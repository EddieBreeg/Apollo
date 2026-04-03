file(GLOB CORE_HEADERS *.h *.hpp)

set(SLANG_CORE_HEADER ${CMAKE_CURRENT_BINARY_DIR}/SlangCore.hpp)

add_custom_command(OUTPUT ${SLANG_CORE_HEADER}
COMMAND ${CMAKE_COMMAND} -DSLANG_FILE="Apollo.Core.slang" -DVARNAME="g_CoreSource" -DOUTPUT_FILE="${SLANG_CORE_HEADER}"
	-P EmbedShader.cmake
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../data/shaders
DEPENDS "../../data/shaders/Apollo.Core.slang" "../../data/shaders/ShaderTemplate.hpp"
)

add_library(CoreSlangHeader INTERFACE ${SLANG_CORE_HEADER})

target_sources(${PROJECT_NAME}Runtime PRIVATE
	App.cpp
	Errno.cpp
	GameTime.cpp
	RNG.cpp
	TypeInfo.cpp
	ULID.cpp
	Window.cpp
	${CORE_HEADERS}
)
target_link_libraries(${PROJECT_NAME}Runtime PRIVATE CoreSlangHeader)