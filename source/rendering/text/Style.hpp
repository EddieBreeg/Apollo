#pragma once

#include <PCH.hpp>

namespace brk::rdr::txt {
	struct TextStyle
	{
		float4 m_FgColor = { 1, 1, 1, 1 };
		float4 m_OutlineColor = { 0, 0, 0, 1 };
		float m_Size = 1.0f; // Normalized text size, in clipspace units
		float m_OutlineThickness = 0.0f;
		float m_Tracking = 1.0f; // Character spacing factor
	};
} // namespace brk::rdr::txt