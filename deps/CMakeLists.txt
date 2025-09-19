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

FetchContent_MakeAvailable(catch2 spdlog sdl3 json)
set(CATCH2_DIR ${catch2_SOURCE_DIR} PARENT_SCOPE)