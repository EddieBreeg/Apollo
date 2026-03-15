file(GLOB UI_HEADERS *.h *.hpp *.inl)

target_sources(${PROJECT_NAME}Runtime PRIVATE
	${UI_HEADERS}
	Context.cpp
	Element.cpp
)