file(GLOB SYSTEM_HEADERS *.h *.hpp)

target_sources(${PROJECT_NAME}Runtime PRIVATE
	InputSystem.cpp
	RegisterCoreSystems.cpp
	SceneLoadingSystem.cpp
	${SYSTEM_HEADERS}
)