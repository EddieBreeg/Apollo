file(GLOB CORE_HEADERS *.h *.hpp)

target_sources(${PROJECT_NAME}Runtime PRIVATE
	App.cpp
	${CORE_HEADERS}
)