#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"
#include "ShaderInfo.hpp"
#include <asset/Asset.hpp>
#include <span>

struct SDL_GPUShader;

namespace apollo::rdr {
	enum class EShaderStage : int8;

	class GraphicsShader : public _internal::HandleWrapper<SDL_GPUShader*>, public IAsset
	{
	public:
		void Swap(GraphicsShader& other) noexcept
		{
			BaseType::Swap(other);
			apollo::Swap(m_Info, other.m_Info);
		}

		GraphicsShader& operator=(GraphicsShader&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		APOLLO_API ~GraphicsShader();

		[[nodiscard]] std::span<const ShaderConstantBlock> GetParameterBlocks() const noexcept
		{
			return { m_Info.m_Blocks, m_Info.m_NumUniformBuffers };
		}

		[[nodiscard]] uint32 GetNumSamplers() const noexcept { return m_Info.m_NumSamplers; }

	protected:
		GraphicsShader() = default;
		GraphicsShader(const ULID& id)
			: IAsset(id)
		{}

		GraphicsShader(GraphicsShader&& other)
			: BaseType(std::move(other))
			, m_Info(std::move(other.m_Info))
		{}

		APOLLO_API GraphicsShader(
			const ULID& id,
			EShaderStage stage,
			const void* code,
			size_t codeLen);

		APOLLO_API GraphicsShader(
			const ULID& id,
			EShaderStage stage,
			std::string_view source,
			const char* entryPoint = "main");

		ShaderInfo m_Info;
	};

	class VertexShader : public GraphicsShader
	{
	public:
		using GraphicsShader::GraphicsShader;
		VertexShader() = default;
		VertexShader(const ULID& id)
			: GraphicsShader(id)
		{}
		VertexShader(const ULID& id, const void* byteCode, size_t codeLen)
			: GraphicsShader(id, EShaderStage::Vertex, byteCode, codeLen)
		{}

		[[nodiscard]] static VertexShader CompileFromSource(
			const ULID& id,
			std::string_view hlsl,
			const char* entryPoint = "main")
		{
			return VertexShader{ id, EShaderStage::Vertex, hlsl, entryPoint };
		}

		GET_ASSET_TYPE_IMPL(EAssetType::VertexShader);
	};

	class FragmentShader : public GraphicsShader
	{
	public:
		using GraphicsShader::GraphicsShader;
		FragmentShader() = default;
		FragmentShader(const ULID& id)
			: GraphicsShader(id)
		{}
		FragmentShader(const ULID& id, const void* byteCode, size_t codeLen)
			: GraphicsShader(id, EShaderStage::Fragment, byteCode, codeLen)
		{}

		[[nodiscard]] static FragmentShader CompileFromSource(
			const ULID& id,
			std::string_view hlsl,
			const char* entryPoint = "main")
		{
			return FragmentShader{ id, EShaderStage::Fragment, hlsl, entryPoint };
		}

		GET_ASSET_TYPE_IMPL(EAssetType::FragmentShader);
	};
} // namespace apollo::rdr