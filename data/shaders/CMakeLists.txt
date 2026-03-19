set(SHADERS_HEADER ${CMAKE_CURRENT_BINARY_DIR}/UiShaders.hpp)
include(Embed)

message("-- Embedding UI Shaders")

EmbedText("./ui/rect.vert.hlsl" VERT_SOURCE g_UiRectVert LITERAL_PREFIX hlsl)
EmbedText("./ui/rect.frag.hlsl" FRAG_SOURCE g_UiRectFrag LITERAL_PREFIX hlsl)

file(WRITE "${SHADERS_HEADER}"
"#pragma once

namespace apollo::data::shaders {
	${VERT_SOURCE}

	${FRAG_SOURCE}
}
")

add_library(BuiltinShaders INTERFACE)
target_include_directories(BuiltinShaders INTERFACE ${CMAKE_CURRENT_BINARY_DIR})