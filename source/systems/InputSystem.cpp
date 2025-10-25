#include "InputSystem.hpp"
#include "InputEventComponents.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <backends/imgui_impl_sdl3.h>
#include <core/App.hpp>
#include <entry/Entry.hpp>
#include <entt/entity/registry.hpp>

namespace {
	const bool* g_KeyStates = nullptr;

	apollo::EAppResult ProcessEvents(SDL_Window* mainWindow, entt::registry& entityWorld)
	{
		using namespace apollo::inputs;

		SDL_Event evt;
		const auto mainWindowId = SDL_GetWindowID(mainWindow);

		while (SDL_PollEvent(&evt))
		{
			entt::entity eventEntity = entityWorld.create();
			entityWorld.emplace<InputEventComponent>(eventEntity);

			ImGui_ImplSDL3_ProcessEvent(&evt);
			switch (evt.type)
			{
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				if (evt.window.windowID != mainWindowId)
					return apollo::EAppResult::Continue;
				[[fallthrough]];
			case SDL_EVENT_QUIT: return apollo::EAppResult::Success;

			case SDL_EVENT_WINDOW_RESIZED:
				if (evt.window.windowID == mainWindowId)
				{
					entityWorld.emplace<WindowResizeEventComponent>(
						eventEntity,
						uint32(evt.window.data1),
						uint32(evt.window.data2));
				}
				break;

			case SDL_EVENT_KEY_DOWN:
				entityWorld.emplace<KeyDownEventComponent>(
					eventEntity,
					EKey(evt.key.key),
					evt.key.repeat);
				break;

			case SDL_EVENT_KEY_UP:
				entityWorld.emplace<KeyUpEventComponent>(eventEntity, EKey(evt.key.key));
				break;

			case SDL_EVENT_MOUSE_MOTION:
				entityWorld.emplace<MouseMotionEventComponent>(
					eventEntity,
					float2{ evt.motion.x, evt.motion.y },
					float2{ evt.motion.xrel, evt.motion.yrel });
				break;
			default: continue;
			}
		}

		return apollo::EAppResult::Continue;
	}
} // namespace

namespace apollo::inputs {
	System::System(App& app)
		: m_App(app)
	{
		g_KeyStates = SDL_GetKeyboardState(nullptr);
	}

	void System::Update(entt::registry& world, const GameTime&)
	{
		for (entt::entity event : world.view<const InputEventComponent>())
			world.destroy(event);

		const EAppResult result = ProcessEvents(m_App.GetMainWindow().GetHandle(), world);
		if (result != EAppResult::Continue)
			m_App.RequestAppQuit();
	}

	const bool* GetKeyStates() noexcept
	{
		return g_KeyStates;
	}
} // namespace apollo::inputs