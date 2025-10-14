#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"
#include "ShaderInfo.hpp"
#include <asset/Asset.hpp>

struct SDL_GPUShader;

namespace apollo::rdr {
	class Shader : public _internal::HandleWrapper<SDL_GPUShader*>, public IAsset
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

		APOLLO_API Shader(const ULID& id, const ShaderInfo& info, const void* code, size_t codeLen);
		APOLLO_API Shader(const ShaderInfo& info, const void* code, size_t codeLen);

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

		APOLLO_API ~Shader();

		[[nodiscard]] EShaderStage GetStage() const noexcept { return m_Stage; }

		GET_ASSET_TYPE_IMPL(EAssetType::Shader);

	private:
		EShaderStage m_Stage = EShaderStage::Invalid;
	};
} // namespace apollo::rdr