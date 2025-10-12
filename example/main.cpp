#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <core/Utf8.hpp>
#include <core/Window.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <entt/entity/registry.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <msdfgen.h>
#include <rendering/Bitmap.hpp>
#include <rendering/Buffer.hpp>
#include <rendering/Device.hpp>
#include <rendering/Material.hpp>
#include <rendering/Pixel.hpp>
#include <rendering/Renderer.hpp>
#include <rendering/Texture.hpp>
#include <rendering/text/BatchRenderer.hpp>
#include <rendering/text/FontAtlas.hpp>
#include <systems/InputEventComponents.hpp>

namespace ImGui {
	void ShowDemoWindow(bool* p_open);

}

namespace brk::demo {
	glm::mat4x4 GetProjMatrix(uint32 width, uint32 height)
	{
		const float xmax = float(width) / height;
		return glm::orthoRH(-xmax, xmax, -1.0f, 1.0f, 0.01f, 100.f);
	}

	constexpr const char* g_DefaultText =
		R"(The quick brown
fox jumps over
the lazy dog.)";

	struct TestSystem
	{
		brk::Window& m_Window;
		brk::rdr::Renderer& m_Renderer;
		brk::AssetRef<brk::rdr::Material> m_Material;
		brk::AssetRef<brk::rdr::txt::FontAtlas> m_Font;
		brk::rdr::txt::Renderer2d m_TextRenderer;
		bool m_GeometryReady = false;

		float m_Scale = 1.0f;

		float m_AntiAliasing = 1.0f;
		glm::mat4x4 m_CamMatrix = glm::identity<glm::mat4x4>();
		glm::mat4x4 m_ModelMatrix = glm::identity<glm::mat4x4>();
		std::string_view m_Text;

		void ProcessWindowResize(entt::registry& world)
		{
			using namespace brk::inputs;
			const auto view = world.view<const WindowResizeEventComponent>();
			if (view->empty())
				return;

			const WindowResizeEventComponent& eventData = *view->begin();

			m_CamMatrix = GetProjMatrix(uint32(eventData.m_Width), uint32(eventData.m_Height));
		}

		TestSystem(brk::Window& window, brk::rdr::Renderer& renderer, std::string_view text)
			: m_Window(window)
			, m_Renderer(renderer)
			, m_Text(text)
		{
			m_ModelMatrix = glm::identity<glm::mat4x4>();

			glm::uvec2 winSize = m_Window.GetSize();
			m_CamMatrix = GetProjMatrix(winSize.x, winSize.y);
		}

		~TestSystem() {}

		void PostInit()
		{
			m_TextRenderer.m_Style.m_Size = 0.2f;

			BRK_LOG_TRACE("Example Post-Init");
			auto* assetManager = AssetManager::GetInstance();
			BRK_ASSERT(assetManager, "Asset manager hasn't been initialized!");

			m_Material = assetManager->GetAsset<brk::rdr::Material>(
				"01K6841M7W2D1J00QKJHHBDJG5"_ulid);
			m_Font = assetManager->GetAsset<brk::rdr::txt::FontAtlas>(
				"01K77QW60RWKYCZFHVP704FV6B"_ulid);
			m_TextRenderer.SetFont(m_Font);

			assetManager->GetAssetLoader().RegisterCallback(
				[this]()
				{
					OnLoadingFinished();
				});
		}

		void OnLoadingFinished()
		{
			const auto& device = m_Renderer.GetDevice();
			BRK_LOG_TRACE("Loading complete");
			if (m_Font->GetState() == EAssetState::Loaded)
			{
				m_TextRenderer.Init(
					device,
					*m_Material->GetVertexShader(),
					*m_Material->GetFragmentShader(),
					SDL_GPUColorTargetDescription{
						.format = SDL_GetGPUSwapchainTextureFormat(
							device.GetHandle(),
							m_Window.GetHandle()),
						.blend_state =
							SDL_GPUColorTargetBlendState{
								.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
								.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
								.color_blend_op = SDL_GPU_BLENDOP_ADD,
								.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
								.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
								.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
								.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
													SDL_GPU_COLORCOMPONENT_G |
													SDL_GPU_COLORCOMPONENT_B |
													SDL_GPU_COLORCOMPONENT_A,
								.enable_blend = true,
							},
					},
					128);
			}
		}

		void DisplayUi()
		{
			ImGui::Begin("Settings");
			ImGui::SliderFloat("Anti-Aliasing Width", &m_AntiAliasing, 0.0f, 5.0f);
			if (ImGui::SliderFloat("Scale", &m_Scale, 0, 1))
			{
				m_ModelMatrix[0].x = m_ModelMatrix[1].y = m_ModelMatrix[2].z = m_Scale;
			}

			m_GeometryReady &= !ImGui::SliderFloat(
				"Outline Thickness",
				&m_TextRenderer.m_Style.m_OutlineThickness,
				0,
				0.5f);
			m_GeometryReady &= !ImGui::DragFloat(
				"Tracking",
				&m_TextRenderer.m_Style.m_Tracking,
				0.001f);
			ImGui::End();
		}

		void Update(entt::registry& world, const brk::GameTime&)
		{
			if (!m_Window) [[unlikely]]
				return;

			auto* swapchainTexture = m_Renderer.GetSwapchainTexture();
			auto* mainCommandBuffer = m_Renderer.GetMainCommandBuffer();

			if (!swapchainTexture || !m_Font || m_Font->GetState() != EAssetState::Loaded)
				return;

			ProcessWindowResize(world);
			DisplayUi();
			m_TextRenderer.StartFrame(mainCommandBuffer);
			if (!m_GeometryReady)
			{
				m_TextRenderer.Clear();
				m_TextRenderer.AddText(m_Text, { 0, 0 }, rdr::txt::Renderer2d::Center);
				m_GeometryReady = true;
			}

			m_TextRenderer.StartRender();

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

			m_TextRenderer.Render(renderPass);

			SDL_EndGPURenderPass(renderPass);
		}
	};

	brk::EAppResult Init(const brk::EntryPoint& entry, brk::App& app)
	{
		spdlog::set_level(spdlog::level::trace);
		const std::span args = entry.m_Args;

		const char* text = args.size() > 2 ? args[2] : g_DefaultText;

		auto& renderer = *brk::rdr::Renderer::GetInstance();
		ImGui::SetCurrentContext(app.GetImGuiContext());
		auto& manager = *brk::ecs::Manager::GetInstance();
		manager.AddSystem<TestSystem>(app.GetMainWindow(), renderer, text);
		return EAppResult::Continue;
	}
} // namespace brk::demo

void brk::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Breakout Example";
	out_entry.m_OnInit = brk::demo::Init;
}