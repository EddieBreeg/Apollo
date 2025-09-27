#pragma once

#include <PCH.hpp>

#include "GameTime.hpp"
#include "Singleton.hpp"
#include "Window.hpp"

struct ImGuiContext;

namespace brk {
	class AssetManager;

	namespace ecs {
		class Manager;
	}

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
		[[nodiscard]] Window& GetMainWindow() noexcept { return m_Window; }
		[[nodiscard]] ImGuiContext* GetImGuiContext() noexcept { return m_ImGuiContext; }

		EAppResult Run();

	private:
		EAppResult Update();

		App(const EntryPoint& entry);
		friend class Singleton<App>;

		static std::unique_ptr<App> s_Instance;

		const EntryPoint& m_EntryPoint;
		Window m_Window;
		rdr::Renderer* m_Renderer = nullptr;
		ecs::Manager* m_ECSManager = nullptr;
		AssetManager* m_AssetManager = nullptr;
		ImGuiContext* m_ImGuiContext = nullptr;

		EAppResult m_Result;
		GameTime m_GameTime;
	};
} // namespace brk