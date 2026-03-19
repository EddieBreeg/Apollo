#pragma once

#include <PCH.hpp>
#include <clay.h>

namespace apollo::rdr::ui {
	class Renderer;
}

namespace apollo::ui {
	class Context
	{
	public:
		APOLLO_API static Context s_Instance;

		Context(const Context&) = delete;
		Context& operator=(const Context&) = delete;
		~Context() { Reset(); }

		APOLLO_API Context& Init(rdr::ui::Renderer& renderer, float width, float height);
		APOLLO_API void Reset();

		APOLLO_API bool SetSize(float width, float height);

		APOLLO_API void BeginLayout() const;
		APOLLO_API void EndLayout() const;

	private:
		Context() = default;

		APOLLO_API void OnError(Clay_ErrorType type, Clay_String errMessage);

		Clay_Dimensions m_CurrentSize = {};
		Clay_Arena m_Arena = {};
		rdr::ui::Renderer* m_Renderer = nullptr;
	};
} // namespace apollo::ui