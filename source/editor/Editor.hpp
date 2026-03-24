#pragma once

#include <PCH.hpp>

#include <entt/entity/registry.hpp>
#include <filesystem>
#include <span>

/** \file Editor.hpp */

namespace apollo::rdr {
	class Context;
}

namespace apollo {
	enum class EAppResult : int8;

	class App;
	class GameTime;

	struct EntryPoint;
} // namespace apollo

/**
 * \namespace apollo::editor
 * \brief The editor library of the engine.
 * \details All tools provided by Apollo to create and edit a game.
 */
namespace apollo::editor {
	/**
	 * \brief Editor system, mostly in charge of handling dev UIs
	 */
	class Editor
	{
	public:
		~Editor() = default;
		Editor(std::span<const char*> args, rdr::Context& renderContext);

		static EAppResult Init(std::span<const char*> args, App&);

		void Update(entt::registry&, const GameTime&);

	private:
		std::filesystem::path m_ProjectPath;
		rdr::Context& m_RenderContext;
	};
} // namespace apollo::editor