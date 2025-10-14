#pragma once

#include <PCH.hpp>

namespace apollo::rdr {
	enum class EShaderStage : int8
	{
		Invalid = -1,
		Vertex,
		Fragment,
		NStages
	};

	struct ShaderInfo
	{
		uint32 m_NumSamplers = 0;
		uint32 m_NumStorageTextures = 0;
		uint32 m_NumStorageBuffers = 0;
		uint32 m_NumUniformBuffers = 0;
		const char* m_EntryPoint = "main";
		EShaderStage m_Stage = EShaderStage::Invalid;

		[[nodiscard]] APOLLO_API static ShaderInfo GetFromByteCode(const void* code, size_t len);
	};
} // namespace apollo::rdr