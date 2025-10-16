#pragma once

#include <PCH.hpp>

#include "FontAtlas.hpp"
#include "Style.hpp"
#include <asset/AssetRef.hpp>
#include <rendering/Buffer.hpp>
#include <rendering/GpuAlign.hpp>
#include <rendering/Material.hpp>

struct SDL_GPUColorTargetDescription;
struct SDL_GPUCommandBuffer;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPURenderPass;
struct SDL_GPUSampler;

namespace apollo::rdr {
	class GPUDevice;
}

namespace apollo::rdr::txt {
	class Renderer2d
	{
	public:
		enum EAnchorPoint
		{
			TopLeft,
			Center,
		};
		struct GlyphQuad
		{
			GPU_ALIGN(float4) m_Rect;
			GPU_ALIGN(float4) m_Uv;
			GPU_ALIGN(float4) m_MainColor;
			GPU_ALIGN(float4) m_OutlineColor;
			GPU_ALIGN(float) m_OutlineThickness;
		};

		Renderer2d() = default;
		APOLLO_API void Init(
			const rdr::GPUDevice& device,
			AssetRef<Material> material,
			uint32 batchSize);

		void SetFont(AssetRef<FontAtlas> font) noexcept { m_Font = std::move(font); }

		[[nodiscard]] bool IsInitialized() const noexcept
		{
			return m_Material && m_Sampler && m_Buffer;
		}

		TextStyle m_Style;

		void Clear() noexcept
		{
			m_Quads.clear();
			m_Dirty = false;
		}

		APOLLO_API void StartFrame(SDL_GPUCommandBuffer* cmdBuffer);
		APOLLO_API void AddText(
			std::string_view str,
			float2 pos,
			EAnchorPoint anchor = EAnchorPoint::Center);

		// Uploads data to the GPU if necessary
		APOLLO_API void StartRender();
		APOLLO_API void Render(SDL_GPURenderPass* renderPass);

		APOLLO_API ~Renderer2d();

	private:
		void Upload();

		SDL_GPUCopyPass* m_CopyPass = nullptr;

		bool m_Dirty = false;

		AssetRef<FontAtlas> m_Font;
		Buffer m_Buffer; // storage buffer for quad instances
		std::vector<GlyphQuad> m_Quads;

		uint32 m_BatchSize;
		AssetRef<Material> m_Material;
		const GPUDevice* m_Device = nullptr;
		SDL_GPUSampler* m_Sampler = nullptr;
	};
} // namespace apollo::rdr::txt