set(SHADERS_HEADER ${OUTPUT_PATH}/UiShaders.hpp)
include(Embed)

message("-- Embedding UI Shaders into ${SHADERS_HEADER}")

EmbedText("./ui/rect.vert.hlsl" VERT_SOURCE g_UiRectVert LITERAL_PREFIX hlsl)
EmbedText("./ui/rect.frag.hlsl" FRAG_SOURCE g_UiRectFrag LITERAL_PREFIX hlsl)

file(WRITE "${SHADERS_HEADER}"
"#pragma once

namespace apollo::data::shaders {
	${VERT_SOURCE}

	${FRAG_SOURCE}
}
")
