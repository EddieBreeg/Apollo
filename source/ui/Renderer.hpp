#pragma once

#include <PCH.hpp>

#include <glm/mat4x4.hpp>
#include <rendering/Batch.hpp>
#include <rendering/GpuAlign.hpp>
#include <rendering/Pixel.hpp>
#include <span>

struct Clay_RenderCommand;

namespace apollo::rdr {
	class Context;

	struct RenderPassSettings;
} // namespace apollo::rdr

namespace apollo::rdr::ui {
	struct UiRect
	{
		GPU_ALIGN(float4) Bounds = {};
		GPU_ALIGN(float4) BgColor = {};
		GPU_ALIGN(float4) CornerRadius = {};

		[[nodiscard]] bool operator!=(const UiRect& other) const noexcept
		{
			return Bounds != other.Bounds || BgColor != other.BgColor ||
				   CornerRadius != other.CornerRadius;
		}
	};
	struct Border
	{
		GPU_ALIGN(float4) Bounds;
		GPU_ALIGN(float4) Color;
		GPU_ALIGN(float4) Width;
		GPU_ALIGN(float4) CornerRadius;

		[[nodiscard]] bool operator!=(const Border& other) const noexcept
		{
			return Bounds != other.Bounds || Color != other.Color || Width != other.Width ||
				   CornerRadius != other.CornerRadius;
		}
	};

	class Renderer
	{
	public:
		APOLLO_API static Renderer s_Instance;

		Renderer() = default;
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(Renderer&&) = delete;
		~Renderer() { Reset(); }

		APOLLO_API void StartFrame();
		APOLLO_API void EndFrame(std::span<const Clay_RenderCommand> commands);

		APOLLO_API bool SetTargetSize(float2 size) noexcept;

		APOLLO_API void Init(rdr::Context& ctx, rdr::EPixelFormat targetFormat);
		APOLLO_API void Reset();

	private:
		float2 m_TargetSize;
		glm::mat4x4 m_ProjMatrix;
		Context* m_RenderContext = nullptr;
		SDL_GPUGraphicsPipeline* m_RectPipeline = nullptr;
		SDL_GPUGraphicsPipeline* m_BorderPipeline = nullptr;
		Batch<UiRect> m_Rectangles;
		Batch<Border> m_Borders;
	};
} // namespace apollo::rdr::ui