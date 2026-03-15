#pragma once

#include <PCH.hpp>
#include <clay.h>

namespace apollo::ui {
	class Context
	{
	public:
		static Context s_Instance;

		Context(const Context&) = delete;
		Context& operator=(const Context&) = delete;
		~Context() { Reset(); }

		APOLLO_API Context& Init(float width, float height);
		APOLLO_API void Reset();

		APOLLO_API bool SetSize(float width, float height);

		APOLLO_API void BeginLayout() const;
		[[nodiscard]] APOLLO_API Clay_RenderCommandArray EndLayout() const;

	private:
		Context() = default;

		APOLLO_API void OnError(Clay_ErrorType type, Clay_String errMessage);

		Clay_Dimensions m_CurrentSize = {};
		Clay_Arena m_Arena = {};
	};

	inline Context Context::s_Instance;
} // namespace apollo::ui