set(_args "")

if(${slang_FIND_REQUIRED})
	list(APPEND _args "REQUIRED")
endif(${slang_FIND_REQUIRED})
if(slang_FIND_QUIETLY)
	list(APPEND _args "QUIET")
endif(slang_FIND_QUIETLY)

if(slang_DIR)
	set(_SLANG_DIR ${slang_DIR})
elseif(DEFINED ENV{SLANG_DIR})
	set(_SLANG_DIR $ENV{SLANG_DIR})
endif()

if(_SLANG_DIR)
	find_package(slang ${_args} PATHS "${_SLANG_DIR}")
endif(_SLANG_DIR)