file(GLOB UI_HEADERS *.h *.hpp *.inl)

target_sources(${PROJECT_NAME}Runtime PRIVATE
	${UI_HEADERS}
	Context.cpp
	Element.cpp
	Renderer.cpp
)
target_link_libraries(${PROJECT_NAME}Runtime PRIVATE BuiltinShaders)