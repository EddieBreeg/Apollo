#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <asset/Scene.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <core/Utf8.hpp>
#include <core/Window.hpp>
#include <ecs/ComponentRegistry.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <entt/entity/registry.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <rendering/Bitmap.hpp>
#include <rendering/Buffer.hpp>
#include <rendering/Device.hpp>
#include <rendering/Material.hpp>
#include <rendering/Pixel.hpp>
#include <rendering/Context.hpp>
#include <rendering/Texture.hpp>
#include <rendering/text/BatchRenderer.hpp>
#include <rendering/text/FontAtlas.hpp>
#include <systems/InputEventComponents.hpp>

namespace ImGui {
	void ShowDemoWindow(bool* p_open);
}

namespace apollo::demo {
	glm::mat4x4 GetProjMatrix(uint32 width, uint32 height)
	{
		const float xmax = float(width) / height;
		return glm::orthoRH(-xmax, xmax, -1.0f, 1.0f, 0.01f, 100.f);
	}

	struct TextComponent
	{
		std::string m_Text;
		AssetRef<rdr::txt::FontAtlas> m_Font;

		static constexpr ecs::ComponentReflection<&TextComponent::m_Text, &TextComponent::m_Font>
			Reflection{
				"text",
				{ "str", "font" },
			};
	};

	struct TestSystem
	{
		apollo::Window& m_Window;
		apollo::rdr::Context& m_RenderContext;
		apollo::AssetRef<rdr::Material> m_Material;
		apollo::AssetRef<Scene> m_Scene;
		apollo::AssetRef<rdr::txt::FontAtlas> m_Font;
		apollo::rdr::txt::Renderer2d m_TextRenderer;
		glm::uvec2 m_WinSize;
		bool m_GeometryReady = false;
		std::atomic_bool m_AssetsReady = false;

		float m_AntiAliasing = 1.0f;
		glm::mat4x4 m_CamMatrix = glm::identity<glm::mat4x4>();
		glm::mat4x4 m_ModelMatrix = glm::identity<glm::mat4x4>();
		std::string_view m_Text;

		void ProcessWindowResize(entt::registry& world)
		{
			using namespace apollo::inputs;
			const auto view = world.view<const WindowResizeEventComponent>();
			if (view->empty())
				return;

			const WindowResizeEventComponent& eventData = *view->begin();
			m_WinSize = { eventData.m_Width, eventData.m_Height };

			m_CamMatrix = GetProjMatrix(eventData.m_Width, eventData.m_Height);
		}

		TestSystem(apollo::Window& window, apollo::rdr::Context& renderer)
			: m_Window(window)
			, m_RenderContext(renderer)
			, m_WinSize(m_Window.GetSize())
		{
			m_ModelMatrix = glm::identity<glm::mat4x4>();

			glm::uvec2 winSize = m_Window.GetSize();
			m_CamMatrix = GetProjMatrix(winSize.x, winSize.y);
		}

		void FetchData(const entt::registry& world)
		{
			const GameObject* object = m_Scene->GetGameObject("01K7VZZSR16FXR2NF8DNYSJQQ4"_ulid);
			const TextComponent& comp = world.get<const TextComponent>(object->m_Entity);
			m_Text = comp.m_Text;
			m_TextRenderer.SetFont((m_Font = comp.m_Font));
		}

		~TestSystem() {}

		void PostInit()
		{
			m_TextRenderer.m_Style.m_Size = 0.2f;

			APOLLO_LOG_TRACE("Example Post-Init");
			auto* assetManager = AssetManager::GetInstance();
			APOLLO_ASSERT(assetManager, "Asset manager hasn't been initialized!");

			assetManager->GetAssetLoader().RegisterCallback(
				[this]()
				{
					OnLoadingFinished();
				});
			m_Material = assetManager->GetAsset<rdr::Material>("01K6841M7W2D1J00QKJHHBDJG5"_ulid);
			m_Scene = assetManager->GetAsset<Scene>("01K7VZZSR16FXR2NF8DNYSJQQ4"_ulid);

			m_TextRenderer.Init(m_RenderContext.GetDevice(), m_Material, 128);
		}

		void OnLoadingFinished()
		{
			APOLLO_LOG_INFO("Loading complete");
			m_AssetsReady = m_Material->GetState() == EAssetState::Loaded &&
							m_Scene->GetState() == EAssetState::Loaded;
		}

		void DisplayUi()
		{
			ImGui::Begin("Settings");
			ImGui::SliderFloat("Anti-Aliasing Width", &m_AntiAliasing, 0.0f, 5.0f);
			m_GeometryReady &= !ImGui::DragFloat("Size", &m_TextRenderer.m_Style.m_Size, 0.01f);
			m_GeometryReady &= !ImGui::DragFloat(
				"Line Spacing",
				&m_TextRenderer.m_Style.m_LineSpacing,
				0.01f);
			m_GeometryReady &= !ImGui::SliderFloat(
				"Kerning",
				&m_TextRenderer.m_Style.m_Kerning,
				0.0f,
				1.0f);

			m_GeometryReady &= !ImGui::SliderFloat(
				"Outline Thickness",
				&m_TextRenderer.m_Style.m_OutlineThickness,
				0,
				0.5f);
			m_GeometryReady &= !ImGui::DragFloat(
				"Tracking",
				&m_TextRenderer.m_Style.m_Tracking,
				0.001f);
			m_GeometryReady &= !ImGui::ColorEdit4(
				"Foreground Color",
				&m_TextRenderer.m_Style.m_FgColor.r);
			m_GeometryReady &= !ImGui::ColorEdit4(
				"Outline Color",
				&m_TextRenderer.m_Style.m_OutlineColor.r);
			ImGui::End();
		}

		void Update(entt::registry& world, const apollo::GameTime&)
		{
			if (!m_Window) [[unlikely]]
				return;

			auto* swapchainTexture = m_RenderContext.GetSwapchainTexture();
			auto* mainCommandBuffer = m_RenderContext.GetMainCommandBuffer();

			if (!swapchainTexture)
				return;

			ProcessWindowResize(world);
			DisplayUi();
			if (m_AssetsReady)
			{
				if (!m_Font)
				{
					FetchData(world);
				}

				m_TextRenderer.StartFrame(mainCommandBuffer);
				if (!m_GeometryReady)
				{
					m_TextRenderer.Clear();
					m_TextRenderer.AddText(m_Text, { 0, 0 }, rdr::txt::Renderer2d::Center);
					float2 size = m_Font->MeasureText(m_Text, m_TextRenderer.m_Style);
					const float ratio = float(m_WinSize.x) / m_WinSize.y;
					glm::uvec2 pixelSize = {
						uint32(0.5f * size.x / ratio * m_WinSize.x),
						uint32(0.5f * size.y * m_WinSize.y),
					};
					APOLLO_LOG_TRACE("Text size: ({}, {})", pixelSize.x, pixelSize.y);
					m_GeometryReady = true;
				}

				m_TextRenderer.StartRender();
			}

			const SDL_GPUColorTargetInfo targetInfo{
				.texture = swapchainTexture,
				.clear_color = { .1f, .1f, .1f, 1 },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.cycle = true,
			};

			SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
				mainCommandBuffer,
				&targetInfo,
				1,
				nullptr);

			SDL_PushGPUVertexUniformData(
				mainCommandBuffer,
				0,
				&m_CamMatrix,
				2 * sizeof(glm::mat4x4));

			if (m_AssetsReady)
				m_TextRenderer.Render(renderPass);

			SDL_EndGPURenderPass(renderPass);
		}
	};

	apollo::EAppResult Init(const apollo::EntryPoint&, apollo::App& app)
	{
		spdlog::set_level(spdlog::level::trace);

		auto* compRegistry = ecs::ComponentRegistry::GetInstance();
		compRegistry->RegisterComponent<TextComponent>();

		auto& renderer = *apollo::rdr::Context::GetInstance();
		ImGui::SetCurrentContext(app.GetImGuiContext());
		auto& manager = *apollo::ecs::Manager::GetInstance();
		manager.AddSystem<TestSystem>(app.GetMainWindow(), renderer);
		return EAppResult::Continue;
	}
} // namespace apollo::demo

void apollo::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Apollo Example";
	out_entry.m_OnInit = apollo::demo::Init;
}