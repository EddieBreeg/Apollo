set(CMAKE_FOLDER "tests")

AddExecutable(${PROJECT_NAME}Tests
SOURCES
	BitmapTests.cpp
	EnumTests.cpp
	HashTests.cpp
	JsonTests.cpp
	QueueTests.cpp
	RetainPtrTests.cpp
	RectTests.cpp
	TypeInfoTests.cpp
	SystemTests.cpp
	TypeInfoTests.cpp
	ULIDTests.cpp
	UniqueFunctionTests.cpp
LINK PRIVATE Breakout::Runtime Catch2::Catch2WithMain
PROPERTIES ${COMMON_PROPERTIES}
OPTIONS PRIVATE ${COMPILER_ARGS}
)

AddTest("Bitmap Tests" "${PROJECT_NAME}Tests" FILTERS "[bitmap]")
AddTest("Enum Tests" "${PROJECT_NAME}Tests" FILTERS "[enums]")
AddTest("Hash Tests" "${PROJECT_NAME}Tests" FILTERS "[hash]")
AddTest("Container Tests" "${PROJECT_NAME}Tests" FILTERS "[containers]")
AddTest("JSON Tests" "${PROJECT_NAME}Tests" FILTERS "[json]")
AddTest("Poly Tests" "${PROJECT_NAME}Tests" FILTERS "[poly]")
AddTest("RetainPtr Tests" "${PROJECT_NAME}Tests" FILTERS "[retain_ptr]")
AddTest("ECS Tests" "${PROJECT_NAME}Tests" FILTERS "[ecs]")
AddTest("RTTI Tests" "${PROJECT_NAME}Tests" FILTERS "[rtti]")
AddTest("ULID Tests" "${PROJECT_NAME}Tests" FILTERS "[ulid]")
AddTest("Util Tests" "${PROJECT_NAME}Tests" FILTERS "[util]")