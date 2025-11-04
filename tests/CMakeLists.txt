set(CMAKE_FOLDER "tests")

AddExecutable(${PROJECT_NAME}Tests
SOURCES
	main.cpp
	AssetLoaderTests.cpp
	BitmapTests.cpp
	ComponentRegistryTests.cpp
	EnumTests.cpp
	GraphicsPipelineTests.cpp
	HashTests.cpp
	JsonTests.cpp
	MathTests.cpp
	MetaTests.cpp
	NumConvTests.cpp
	QueueTests.cpp
	RetainPtrTests.cpp
	RectTests.cpp
	TypeInfoTests.cpp
	SceneLoadingTests.cpp
	SystemTests.cpp
	ThreadPoolTests.cpp
	TypeInfoTests.cpp
	ULIDTests.cpp
	UniqueFunctionTests.cpp
	UtilTests.cpp
	VertexTests.cpp
LINK PRIVATE Apollo::Runtime Catch2::Catch2 SDL3::SDL3
PROPERTIES ${COMMON_PROPERTIES}
OPTIONS PRIVATE ${COMPILER_ARGS}
)

AddTest("AssetLoader Tests" "${PROJECT_NAME}Tests" FILTERS "[asset_loader][mt]")
AddTest("Bitmap Tests" "${PROJECT_NAME}Tests" FILTERS "[bitmap]")
AddTest("Enum Tests" "${PROJECT_NAME}Tests" FILTERS "[enums]")
AddTest("Hash Tests" "${PROJECT_NAME}Tests" FILTERS "[hash]")
AddTest("Container Tests" "${PROJECT_NAME}Tests" FILTERS "[containers]")
AddTest("JSON Tests" "${PROJECT_NAME}Tests" FILTERS "[json]")
AddTest("NumConv Tests" "${PROJECT_NAME}Tests" FILTERS "[num_conv]")
AddTest("Poly Tests" "${PROJECT_NAME}Tests" FILTERS "[poly]")
AddTest("RetainPtr Tests" "${PROJECT_NAME}Tests" FILTERS "[retain_ptr]")
AddTest("ECS Tests" "${PROJECT_NAME}Tests" FILTERS "[ecs]")
AddTest("RTTI Tests" "${PROJECT_NAME}Tests" FILTERS "[rtti]")
AddTest("Multi-Threading Tests" "${PROJECT_NAME}Tests" FILTERS "[mt]")
AddTest("ULID Tests" "${PROJECT_NAME}Tests" FILTERS "[ulid]")
AddTest("Util Tests" "${PROJECT_NAME}Tests" FILTERS "[util]")
AddTest("Math Tests" "${PROJECT_NAME}Tests" FILTERS "[math]")
AddTest("Rendering Tests" "${PROJECT_NAME}Tests" FILTERS "[rdr]")
AddTest("Scene Management" "${PROJECT_NAME}Tests" FILTERS "[scene]")