#pragma once

#include <PCH.hpp>

#include <span>
#include <filesystem>

namespace brk {
	enum class EAppResult : int8;

	class App;

	struct EntryPoint;
} // namespace brk

namespace brk::editor {
	class Editor
	{
	public:
		~Editor() = default;
		Editor(std::span<const char*> args);

		static EAppResult Init(const EntryPoint&, App&);

		void Update();

	private:
		std::filesystem::path m_ProjectPath;
	};
} // namespace brk::editor