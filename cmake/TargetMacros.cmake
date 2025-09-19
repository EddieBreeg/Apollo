function(AddLibrary NAME)
	cmake_parse_arguments(PARSE_ARGV 0 target "" "TYPE;EXPORT_MACRO" "SOURCES;INCLUDES;PROPERTIES;DEFINITIONS;OPTIONS;LINK")
	add_library(${NAME} ${target_TYPE} ${target_SOURCES})

	if(target_PROPERTIES)
		set_target_properties(${NAME} PROPERTIES ${target_PROPERTIES})
	endif(target_PROPERTIES)

	if(target_DEFINITIONS)
		target_compile_definitions(${NAME} ${target_DEFINITIONS})
	endif(target_DEFINITIONS)

	if(target_OPTIONS)
		target_compile_options(${NAME} ${target_OPTIONS})
	endif(target_OPTIONS)

	if(target_EXPORT_MACRO)
		include(GenerateExportHeader)
		generate_export_header(${NAME}
			EXPORT_MACRO_NAME ${target_EXPORT_MACRO}
		)
		target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
	endif(target_EXPORT_MACRO)

	if(target_INCLUDES)
		target_include_directories(${NAME} ${target_INCLUDES})
	endif(target_INCLUDES)

	if(target_LINK)
		target_link_libraries(${NAME} ${target_LINK})
	endif(target_LINK)
endfunction(AddLibrary)

function(AddExecutable NAME)
	cmake_parse_arguments(PARSE_ARGV 0 target "" "" "SOURCES;INCLUDES;PROPERTIES;DEFINITIONS;OPTIONS;LINK")
	add_executable(${NAME} ${target_TYPE} ${target_SOURCES})

	if(target_PROPERTIES)
		set_target_properties(${NAME} PROPERTIES ${target_PROPERTIES})
	endif(target_PROPERTIES)

	if(target_DEFINITIONS)
		target_compile_definitions(${NAME} ${target_DEFINITIONS})
	endif(target_DEFINITIONS)

	if(target_OPTIONS)
		target_compile_options(${NAME} ${target_OPTIONS})
	endif(target_OPTIONS)

	if(target_INCLUDES)
		target_include_directories(${NAME} ${target_INCLUDES})
	endif(target_INCLUDES)

	if(target_LINK)
		target_link_libraries(${NAME} ${target_LINK})
	endif(target_LINK)
endfunction(AddExecutable)