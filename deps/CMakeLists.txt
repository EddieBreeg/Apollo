set(CMAKE_FOLDER "deps")

include(FetchContent)

# common dependencies

FetchContent_Declare(spdlog
	GIT_REPOSITORY git@github.com:gabime/spdlog.git
	GIT_TAG v1.15.3
)

FetchContent_Declare(catch2
	GIT_REPOSITORY git@github.com:catchorg/Catch2.git
	GIT_TAG v3.8.1
)

FetchContent_Declare(json
	GIT_REPOSITORY git@github.com:nlohmann/json.git
	GIT_TAG v3.12.0
)

FetchContent_Declare(sdl3
	GIT_REPOSITORY git@github.com:libsdl-org/SDL.git
	GIT_TAG  release-3.2.16
)

FetchContent_Declare(imgui
	GIT_REPOSITORY git@github.com:ocornut/imgui.git
	GIT_TAG v1.92.3-docking
)

FetchContent_Declare(glm
	GIT_REPOSITORY git@github.com:g-truc/glm.git
	GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(catch2 spdlog sdl3 json imgui glm)
set(CATCH2_DIR ${catch2_SOURCE_DIR} PARENT_SCOPE)

file(GLOB IMGUI_HEADERS "${imgui_SOURCE_DIR}/*.h")

AddLibrary(imgui SOURCES
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	$<$<NOT:$<CONFIG:Release>>:${imgui_SOURCE_DIR}/imgui_demo.cpp>
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.h
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.h
	${IMGUI_HEADERS}
INCLUDES PUBLIC ${imgui_SOURCE_DIR}
LINK PRIVATE SDL3::SDL3
)