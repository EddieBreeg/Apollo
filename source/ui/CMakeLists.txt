file(GLOB UI_HEADERS *.h *.hpp *.inl)

set(EmbedCommand ${CMAKE_COMMAND}
	-DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
	-DOUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}
	-P EmbedUi.cmake)

set(HLSL_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../../data/shaders/ui/rect.vert.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/../../data/shaders/ui/rect.frag.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/../../data/shaders/ui/border.vert.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/../../data/shaders/ui/border.frag.hlsl
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UiShaders.hpp
	COMMAND ${EmbedCommand}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../data/shaders
	DEPENDS ${HLSL_SOURCES}
)
add_library(UiShaders INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/UiShaders.hpp)
target_include_directories(UiShaders INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

target_sources(${PROJECT_NAME}Runtime PRIVATE
	${UI_HEADERS}
	Context.cpp
	Element.cpp
	Renderer.cpp
	${CMAKE_CURRENT_BINARY_DIR}/UiShaders.hpp
)
target_link_libraries(${PROJECT_NAME}Runtime PRIVATE UiShaders)