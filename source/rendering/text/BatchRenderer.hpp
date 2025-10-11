#pragma once

#include <PCH.hpp>

#include "FontAtlas.hpp"
#include "Style.hpp"
#include <asset/AssetRef.hpp>
#include <rendering/Buffer.hpp>
#include <rendering/Material.hpp>

struct SDL_GPUColorTargetDescription;
struct SDL_GPUCommandBuffer;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPURenderPass;
struct SDL_GPUSampler;

namespace brk::rdr {
	class GPUDevice;
}

namespace brk::rdr::txt {
	class Renderer2d
	{
	public:
		enum EAnchorPoint
		{
			TopLeft,
			Center,
		};

		Renderer2d() = default;
		BRK_API void Init(
			const rdr::GPUDevice& device,
			const Shader& vertexShader,
			const Shader& fragmentShader,
			const SDL_GPUColorTargetDescription& targetDesc,
			uint32 batchSize);

		void SetFont(AssetRef<FontAtlas> font) noexcept { m_Font = std::move(font); }

		[[nodiscard]] bool IsInitialized() const noexcept { return m_Pipeline; }

		TextStyle m_Style;

		void Clear() noexcept
		{
			m_Quads.clear();
			m_Dirty = false;
		}

		BRK_API void StartFrame(SDL_GPUCommandBuffer* cmdBuffer);
		BRK_API void AddText(
			std::string_view str,
			float2 pos,
			EAnchorPoint anchor = EAnchorPoint::Center);

		// Uploads data to the GPU if necessary
		BRK_API void StartRender();
		BRK_API void Render(SDL_GPURenderPass* renderPass);

		BRK_API ~Renderer2d();

	private:
		void Upload();
		struct alignas(16) GlyphQuad
		{
			float4 m_Rect;
			float4 m_Uv;
			float4 m_MainColor;
			float4 m_OutlineColor;
			float m_OutlineThickness;
		};

		SDL_GPUCopyPass* m_CopyPass = nullptr;

		bool m_Dirty = false;

		AssetRef<FontAtlas> m_Font;
		Buffer m_Buffer; // storage buffer for quad instances
		std::vector<GlyphQuad> m_Quads;

		uint32 m_BatchSize;
		AssetRef<Material> m_Material;
		const GPUDevice* m_Device = nullptr;
		SDL_GPUGraphicsPipeline* m_Pipeline = nullptr;
		SDL_GPUSampler* m_Sampler = nullptr;
	};
} // namespace brk::rdr::txt