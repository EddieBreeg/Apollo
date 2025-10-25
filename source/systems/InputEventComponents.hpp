#pragma once

#include <PCH.hpp>
#include <core/KeyCodes.hpp>

namespace apollo::inputs {
	struct WindowResizeEventComponent
	{
		uint32 m_Width = 0, m_Height = 0;
	};

	struct MouseMotionEventComponent
	{
		float2 m_Position;
		float2 m_Motion;
	};

	struct KeyDownEventComponent
	{
		EKey m_Key = EKey::Unknown;
		bool m_Repeat = false;
	};

	struct KeyUpEventComponent
	{
		EKey m_Key = EKey::Unknown;
	};

	// Doesn't contain any data, but gets attached to all entities which have any of the components
	// defined above
	struct InputEventComponent
	{};
} // namespace apollo::inputs