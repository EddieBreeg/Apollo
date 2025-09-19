if(${PROJECT_NAME}_BACKEND STREQUAL "Vulkan")
	find_package(Vulkan REQUIRED COMPONENTS glslc)
endif(${PROJECT_NAME}_BACKEND STREQUAL "Vulkan")


function(AddShader NAME STAGE INPUT_FILE OUTPUT_FILE)
	cmake_parse_arguments(PARSE_ARGV 0 tgt "DEBUG" "ENTRY_POINT" "COMPILE_OPTIONS")

	if(${PROJECT_NAME}_BACKEND STREQUAL "D3D12")
		if(${STAGE} MATCHES "vert")
			set(STAGE vs_6_0)
		elseif(${STAGE} MATCHES "frag")
			set(STAGE ps_6_0)
		elseif(${STAGE} MATCHES "comp")
			set(STAGE cs_6_0)
		else()
			message(FATAL_ERROR "-- Invalid shader stage ${STAGE}")
		endif(${STAGE} MATCHES "vert")

		set(COMPILE_OPTIONS -T ${STAGE} ${tgt_COMPILE_OPTIONS})

		if(tgt_ENTRY_POINT)
			list(APPEND COMPILE_OPTIONS -E ${tgt_ENTRY_POINT})
		endif(tgt_ENTRY_POINT)

		if(tgt_DEBUG)
			list(APPEND COMPILE_OPTIONS -Zi -Qembed_debug)
		endif(tgt_DEBUG)

		set(COMPILE_COMMAND dxc ${COMPILE_OPTIONS} -Fo "${OUTPUT_FILE}" "${INPUT_FILE}")
	elseif(${PROJECT_NAME}_BACKEND STREQUAL "Vulkan")
		set(COMPILE_OPTIONS -fshader-stage=${STAGE} ${tgt_COMPILE_OPTIONS})

		if(tgt_ENTRY_POINT)
			list(APPEND COMPILE_OPTIONS -fentry-point=${tgt_ENTRY_POINT})
		endif(tgt_ENTRY_POINT)

		if(tgt_DEBUG)
			list(APPEND COMPILE_OPTIONS -g)
		endif(tgt_DEBUG)

		set(COMPILE_COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${COMPILE_OPTIONS} -o "${OUTPUT_FILE}" "${INPUT_FILE}")
	endif(${PROJECT_NAME}_BACKEND STREQUAL "D3D12")
	
	add_custom_target(${NAME} COMMAND ${COMPILE_COMMAND} BYPRODUCTS ${OUTPUT_FILE} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction(AddShader NAME INPUT_FILE OUTPUT_FILE STAGE)
