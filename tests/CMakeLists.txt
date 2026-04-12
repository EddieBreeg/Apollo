set(CMAKE_FOLDER "tests")

find_package(slang REQUIRED)

AddExecutable(${PROJECT_NAME}Tests
SOURCES
	main.cpp
	AssetLoaderTests.cpp
	AssetLoadTaskTests.cpp
	BitmapTests.cpp
	BitSpanTests.cpp
	BlobTests.cpp
	CoroutineTests.cpp
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
	SlangTests.cpp
	SystemTests.cpp
	ThreadPoolTests.cpp
	TypeInfoTests.cpp
	ULIDTests.cpp
	UniqueFunctionTests.cpp
	UtilTests.cpp
	VertexTests.cpp
LINK PRIVATE Apollo::Runtime Catch2::Catch2 SDL3::SDL3 slang::slang
PROPERTIES ${COMMON_PROPERTIES}
OPTIONS PRIVATE ${COMPILER_ARGS}
DEFINITIONS PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

AddTest("All Tests" "${PROJECT_NAME}Tests")

AddTest("AssetLoader Tests" "${PROJECT_NAME}Tests" FILTERS "[asset_loader][mt]")
AddTest("AssetLoadTask Tests" "${PROJECT_NAME}Tests" FILTERS "[asset_load_task]")
AddTest("Bitmap Tests" "${PROJECT_NAME}Tests" FILTERS "[bitmap]")
AddTest("BitSpan Tests" "${PROJECT_NAME}Tests" FILTERS "[bitspan]")
AddTest("Blob Tests" "${PROJECT_NAME}Tests" FILTERS "[blob]")
AddTest("Coroutine Tests" "${PROJECT_NAME}Tests" FILTERS "[coroutine]")
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
AddTest("Shader Tests" "${PROJECT_NAME}Tests" FILTERS "[shaders]")
AddTest("ULID Tests" "${PROJECT_NAME}Tests" FILTERS "[ulid]")
AddTest("Util Tests" "${PROJECT_NAME}Tests" FILTERS "[util]")
AddTest("Math Tests" "${PROJECT_NAME}Tests" FILTERS "[math]")
AddTest("Rendering Tests" "${PROJECT_NAME}Tests" FILTERS "[rdr]")
AddTest("Scene Management" "${PROJECT_NAME}Tests" FILTERS "[scene]")