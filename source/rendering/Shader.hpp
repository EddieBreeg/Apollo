#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"
#include "ShaderInfo.hpp"
#include <asset/Asset.hpp>

struct SDL_GPUShader;

namespace brk::rdr {
	class BRK_API Shader : public _internal::HandleWrapper<SDL_GPUShader*>, public IAsset
	{
	public:
		Shader() = default;
		Shader(const ULID& id)
			: IAsset(id)
		{}

		Shader(Shader&& other)
			: BaseType(std::move(other))
			, m_Stage(other.m_Stage)
		{
			other.m_Stage = EShaderStage::Invalid;
		}

		Shader(const ULID& id, const ShaderInfo& info, const void* code, size_t codeLen);
		Shader(const ShaderInfo& info, const void* code, size_t codeLen);

		void Swap(Shader& other) noexcept
		{
			BaseType::Swap(other);
			std::swap(m_Stage, other.m_Stage);
		}

		Shader& operator=(Shader&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		~Shader();

		GET_ASSET_TYPE_IMPL(EAssetType::Shader);

	private:
		EShaderStage m_Stage = EShaderStage::Invalid;
	};
} // namespace brk::rdr