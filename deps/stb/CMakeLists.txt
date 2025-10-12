add_library(stb_image INTERFACE stb_image.h)
add_library(stb_image_write INTERFACE stb_image_write.h)

target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(stb_image_write INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})