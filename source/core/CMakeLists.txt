file(GLOB CORE_HEADERS *.h *.hpp)

target_sources(${PROJECT_NAME}Runtime PRIVATE
	App.cpp
	Errno.cpp
	RNG.cpp
	TypeInfo.cpp
	ULID.cpp
	Window.cpp
	${CORE_HEADERS}
)