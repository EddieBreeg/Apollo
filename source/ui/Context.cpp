#ifndef CLAY_IMPLEMENTATION
#define CLAY_IMPLEMENTATION
#include "Context.hpp"
#endif
#include <core/Log.hpp>

namespace apollo::ui {
	Context& Context::Init(float w, float h)
	{
		DEBUG_CHECK(!m_Arena.memory)
		{
			APOLLO_LOG_WARN("Called on ui::Context but context has already been initialized");
			return *this;
		}

		m_Arena.capacity = Clay_MinMemorySize();
		m_Arena.memory = new char[m_Arena.capacity];
		m_CurrentSize = Clay_Dimensions{ .width = w, .height = h };

		Clay_Initialize(
			m_Arena,
			m_CurrentSize,
			Clay_ErrorHandler{
				.errorHandlerFunction =
					[](Clay_ErrorData data)
				{
					static_cast<Context*>(data.userData)->OnError(data.errorType, data.errorText);
				},
				.userData = this,
			});
		APOLLO_LOG_TRACE("Initialized UI context");

		return *this;
	}

	void Context::Reset()
	{
		if (m_Arena.memory)
		{
			delete[] m_Arena.memory;
			m_Arena = {};
		}
		m_CurrentSize = {};
	}

	bool Context::SetSize(float w, float h)
	{
		if (w != m_CurrentSize.width || h != m_CurrentSize.height)
		{
			Clay_SetLayoutDimensions(m_CurrentSize = { w, h });
			return true;
		}
		return false;
	}

	void Context::BeginLayout() const
	{
		Clay_BeginLayout();
	}

	Clay_RenderCommandArray Context::EndLayout() const
	{
		return Clay_EndLayout();
	}

	void Context::OnError(Clay_ErrorType type, Clay_String message)
	{
		(void)type;
		APOLLO_LOG_ERROR("[clay error] {:.{}}", message.chars, message.length);
	}
} // namespace apollo::ui

#undef CLAY_IMPLEMENTATION