#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"
#include "ShaderInfo.hpp"
#include <asset/Asset.hpp>

struct SDL_GPUShader;

namespace apollo::rdr {
	enum class EShaderStage : int8;

	class GraphicsShader : public _internal::HandleWrapper<SDL_GPUShader*>, public IAsset
	{
	public:
		void Swap(GraphicsShader& other) noexcept { BaseType::Swap(other); }

		GraphicsShader& operator=(GraphicsShader&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		APOLLO_API ~GraphicsShader();

	protected:
		GraphicsShader() = default;
		GraphicsShader(const ULID& id)
			: IAsset(id)
		{}

		GraphicsShader(GraphicsShader&& other)
			: BaseType(std::move(other))
		{}

		APOLLO_API GraphicsShader(
			const ULID& id,
			EShaderStage stage,
			const void* code,
			size_t codeLen);
	};

	class VertexShader : public GraphicsShader
	{
	public:
		VertexShader() = default;
		VertexShader(const ULID& id)
			: GraphicsShader(id)
		{}
		VertexShader(const ULID& id, const void* byteCode, size_t codeLen)
			: GraphicsShader(id, EShaderStage::Vertex, byteCode, codeLen)
		{}

		GET_ASSET_TYPE_IMPL(EAssetType::VertexShader);
	};

	class FragmentShader : public GraphicsShader
	{
	public:
		FragmentShader() = default;
		FragmentShader(const ULID& id)
			: GraphicsShader(id)
		{}
		FragmentShader(const ULID& id, const void* byteCode, size_t codeLen)
			: GraphicsShader(id, EShaderStage::Fragment, byteCode, codeLen)
		{}
		GET_ASSET_TYPE_IMPL(EAssetType::FragmentShader);
	};
} // namespace apollo::rdr