set(CMAKE_FOLDER "deps")

add_subdirectory(Catch2)
add_subdirectory(glm)
add_subdirectory(SDL)
add_subdirectory(spdlog)
add_subdirectory(stb)

if(${PROJECT_NAME}_BACKEND STREQUAL "Vulkan")
	add_subdirectory(SPIRV-Reflect)
endif(${PROJECT_NAME}_BACKEND STREQUAL "Vulkan")

file(GLOB IMGUI_HEADERS "imgui/*.h")

AddLibrary(imgui SOURCES
	imgui/imgui.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	$<$<NOT:$<CONFIG:Release>>:imgui/imgui_demo.cpp>
	imgui/backends/imgui_impl_sdl3.cpp
	imgui/backends/imgui_impl_sdlgpu3.cpp
	imgui/backends/imgui_impl_sdl3.h
	imgui/backends/imgui_impl_sdlgpu3.h
	${IMGUI_HEADERS}
INCLUDES PUBLIC imgui
LINK PRIVATE SDL3::SDL3
)