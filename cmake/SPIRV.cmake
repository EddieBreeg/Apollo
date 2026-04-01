function(FetchSpirvRepos)
	include(FetchContent)
	FetchContent_Declare(
		spirv-reflect
		GIT_REPOSITORY git@github.com:KhronosGroup/SPIRV-Reflect.git
		GIT_TAG vulkan-sdk-1.4.328.1
	)

	FetchContent_MakeAvailable(
		spirv-reflect)
endfunction(FetchSpirvRepos)
