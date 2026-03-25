#pragma once

#include <PCH.hpp>
#include <core/KeyCodes.hpp>

/** \file InputEventComponents.hpp */

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

	/** \brief Generic input event
	 * \details This component doesn't hold any data on its own, but it gets attached to all input
	 * event entities
	 */
	struct InputEventComponent
	{};
} // namespace apollo::inputs