set(GBENCH_DIR ${CMAKE_CURRENT_BINARY_DIR}/bench)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

if(NOT IS_DIRECTORY ${GBENCH_DIR})
	message("-- Fetching google benchmark...")
endif(NOT IS_DIRECTORY ${GBENCH_DIR})

include(FetchContent)
FetchContent_Declare(google_benchmark
	GIT_REPOSITORY git@github.com:google/benchmark.git
	GIT_TAG main
	SOURCE_DIR ${GBENCH_DIR}
)
FetchContent_MakeAvailable(google_benchmark)

AddExecutable(${PROJECT_NAME}Benchmarks SOURCES MemoryBenchmarks.cpp
	OPTIONS PRIVATE ${COMPILER_ARGS}
	PROPERTIES ${COMMON_PROPERTIES}
	LINK PRIVATE benchmark::benchmark_main ${PROJECT_NAME}::Runtime
)