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
			apollo::Swap(m_NumConstantBlocks, other.m_NumConstantBlocks);
			apollo::Swap(m_ConstantBlocks, other.m_ConstantBlocks);
		}

		GraphicsShader& operator=(GraphicsShader&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		APOLLO_API ~GraphicsShader();

		[[nodiscard]] std::span<const ShaderConstantBlock> GetParameterBlocks() const noexcept
		{
			return { m_ConstantBlocks, m_NumConstantBlocks };
		}

	protected:
		GraphicsShader() = default;
		GraphicsShader(const ULID& id)
			: IAsset(id)
		{}

		GraphicsShader(GraphicsShader&& other)
			: BaseType(std::move(other))
			, m_NumConstantBlocks(other.m_NumConstantBlocks)
		{
			for (uint32 i = 0; i < m_NumConstantBlocks; ++i)
				m_ConstantBlocks[i] = std::move(other.m_ConstantBlocks[i]);
			other.m_NumConstantBlocks = 0;
		}

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

		uint32 m_NumConstantBlocks = 0;
		ShaderConstantBlock m_ConstantBlocks[4];
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