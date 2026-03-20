set(SHADERS_HEADER ${OUTPUT_PATH}/UiShaders.hpp)
include(Embed)

message("-- Embedding UI Shaders into ${SHADERS_HEADER}")

EmbedText("./ui/rect.vert.hlsl" VERT_SOURCE1 g_UiRectVert LITERAL_PREFIX hlsl)
EmbedText("./ui/rect.frag.hlsl" FRAG_SOURCE1 g_UiRectFrag LITERAL_PREFIX hlsl)
EmbedText("./ui/border.vert.hlsl" VERT_SOURCE2 g_UiBorderVert LITERAL_PREFIX hlsl)
EmbedText("./ui/border.frag.hlsl" FRAG_SOURCE2 g_UiBorderFrag LITERAL_PREFIX hlsl)

file(WRITE "${SHADERS_HEADER}"
"#pragma once

namespace apollo::data::shaders {
	${VERT_SOURCE1}

	${FRAG_SOURCE1}

	${VERT_SOURCE2}

	${FRAG_SOURCE2}
}
")
