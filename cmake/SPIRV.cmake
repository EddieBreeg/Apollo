function(FetchSpirvRepos)
	include(FetchContent)
	FetchContent_Declare(
		spirv-headers
		GIT_REPOSITORY git@github.com:KhronosGroup/SPIRV-Headers.git
		GIT_TAG vulkan-sdk-1.4.328.1
	)
	FetchContent_Declare(
		spirv-reflect
		GIT_REPOSITORY git@github.com:KhronosGroup/SPIRV-Reflect.git
		GIT_TAG vulkan-sdk-1.4.328.1
	)
	FetchContent_Declare(
		spirv-tools
		GIT_REPOSITORY git@github.com:KhronosGroup/SPIRV-Tools.git
		GIT_TAG vulkan-sdk-1.4.328.1
	)
	FetchContent_Declare(
		glslang
		GIT_REPOSITORY git@github.com:KhronosGroup/glslang.git
		GIT_TAG vulkan-sdk-1.4.328.1
	)

	FetchContent_MakeAvailable(spirv-headers spirv-reflect spirv-tools glslang)
endfunction(FetchSpirvRepos)
