function(AddTest NAME)
	set(MultiValueKeyWords
		SOURCES
		DEPENDENCIES
		PROPERTIES
		COMPILER_OPTIONS
		DEFINITIONS
		COMMAND_ARGUMENTS
	)
	cmake_parse_arguments(PARSE_ARGV 0 test "" "WORKING_DIRECTORY" "${MultiValueKeyWords}")
	add_executable(${NAME} ${test_SOURCES})
	if(test_PROPERTIES)
		set_target_properties(${NAME} PROPERTIES ${test_PROPERTIES})
	endif(test_PROPERTIES)

	if(test_DEPENDENCIES)
		target_link_libraries(${NAME} PRIVATE ${test_DEPENDENCIES} Catch2::Catch2WithMain)
	endif(test_DEPENDENCIES)
	
	if(test_COMPILER_OPTIONS)
		target_compile_options(${NAME} PRIVATE ${test_COMPILER_OPTIONS})
	endif(test_COMPILER_OPTIONS)

	if(test_DEFINITIONS)
		target_compile_definitions(${NAME} PRIVATE ${test_DEFINITIONS})
	endif(test_DEFINITIONS)
	
	add_test(NAME ${NAME} COMMAND $<TARGET_FILE:${NAME}> ${test_COMMAND_ARGUMENTS}
		WORKING_DIRECTORY ${test_WORKING_DIRECTORY}
	)
endfunction(AddTest NAME)
