#pragma once

#include <PCH.hpp>

#include "Singleton.hpp"
#include "Window.hpp"

namespace brk {
	namespace rdr {
		class Renderer;
	}

	enum class EAppResult : int8;

	struct EntryPoint;

	class BRK_API App : public Singleton<App>
	{
	public:
		~App();
		[[nodiscard]] EAppResult GetResultCode() const noexcept { return m_Result; }
		EAppResult Run();

	private:
		App(const EntryPoint& entry);
		friend class Singleton<App>;

		static std::unique_ptr<App> s_Instance;

		const EntryPoint& m_EntryPoint;
		Window m_Window;
		rdr::Renderer* m_Renderer = nullptr;
		EAppResult m_Result;
	};
} // namespace brk