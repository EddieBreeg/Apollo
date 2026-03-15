add_library(Clay INTERFACE clay.h)
target_include_directories(Clay INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Clay::Clay ALIAS Clay)