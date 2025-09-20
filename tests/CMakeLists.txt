set(CMAKE_FOLDER "tests")

AddTest(${PROJECT_NAME}Tests
SOURCES
	EnumTests.cpp
	TypeInfoTests.cpp
	SystemTests.cpp
DEPENDENCIES Breakout::Runtime
PROPERTIES CXX_STANDARD 20 COMPILE_WARNING_AS_ERROR ON
)

add_custom_target(RunECSTests COMMAND
	$<TARGET_FILE:${PROJECT_NAME}Tests> "[ecs]"
)