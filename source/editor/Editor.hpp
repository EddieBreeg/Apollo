#pragma once

#include <PCH.hpp>

#include <entt/entity/registry.hpp>
#include <filesystem>
#include <span>

namespace apollo::rdr {
	class Context;
}

namespace apollo {
	enum class EAppResult : int8;

	class App;
	class GameTime;

	struct EntryPoint;
} // namespace apollo

namespace apollo::editor {
	class Editor
	{
	public:
		~Editor() = default;
		Editor(std::span<const char*> args, rdr::Context& renderContext);

		static EAppResult Init(const EntryPoint&, App&);

		void Update(entt::registry&, const GameTime&);

	private:
		std::filesystem::path m_ProjectPath;
		rdr::Context& m_RenderContext;
	};
} // namespace apollo::editor