file(GLOB UI_HEADERS *.h *.hpp *.inl)


add_custom_command(OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/SlangUiRect.hpp
		${CMAKE_CURRENT_BINARY_DIR}/SlangUiBorder.hpp
	COMMAND ${CMAKE_COMMAND} -DVARNAME=g_UiRectSource -DSLANG_FILE=UiRect.slang
		-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/SlangUiRect.hpp -P EmbedShader.cmake
	COMMAND ${CMAKE_COMMAND} -DVARNAME=g_UiBorderSource -DSLANG_FILE=UiBorder.slang
		-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/SlangUiBorder.hpp -P EmbedShader.cmake
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../data/shaders
	DEPENDS
		../../data/shaders/UiRect.slang
		../../data/shaders/UiBorder.slang
		../../data/shaders/ShaderTemplate.hpp
)
add_library(UiShaders INTERFACE
	${CMAKE_CURRENT_BINARY_DIR}/SlangUiRect.hpp
	${CMAKE_CURRENT_BINARY_DIR}/SlangUiBorder.hpp
)
target_include_directories(UiShaders INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

target_sources(${PROJECT_NAME}Runtime PRIVATE
	${UI_HEADERS}
	Context.cpp
	Element.cpp
	Renderer.cpp
)
target_link_libraries(${PROJECT_NAME}Runtime PRIVATE UiShaders)