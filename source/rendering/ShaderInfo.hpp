#pragma once

#include <PCH.hpp>
#include <memory>
#include <string>

namespace apollo::rdr {
	enum class EShaderStage : int8
	{
		Invalid = -1,
		Vertex,
		Fragment,
		NStages
	};

	struct ShaderConstant
	{
		enum EType : int8
		{
			Invalid = -1,

			Float,
			Float2,
			Float3,
			Float4,

			Float2x2,
			Float2x3,
			Float2x4,
			Float3x2,
			Float3x3,
			Float3x4,
			Float4x2,
			Float4x3,
			Float4x4,

			Int,
			Int2,
			Int3,
			Int4,

			Int2x2,
			Int2x3,
			Int2x4,
			Int3x2,
			Int3x3,
			Int3x4,
			Int4x2,
			Int4x3,
			Int4x4,

			NTypes
		};

		[[nodiscard]] static constexpr const char* GetTypeName(EType type) noexcept
		{
#define TYPE_CASE(Name)                                                                            \
	case rdr::ShaderConstant::Name: return #Name
			switch (type)
			{
				TYPE_CASE(Float);
				TYPE_CASE(Float2);
				TYPE_CASE(Float3);
				TYPE_CASE(Float4);
				TYPE_CASE(Float2x2);
				TYPE_CASE(Float2x3);
				TYPE_CASE(Float2x4);
				TYPE_CASE(Float3x2);
				TYPE_CASE(Float3x3);
				TYPE_CASE(Float3x4);
				TYPE_CASE(Float4x2);
				TYPE_CASE(Float4x3);
				TYPE_CASE(Float4x4);
				TYPE_CASE(Int);
				TYPE_CASE(Int2);
				TYPE_CASE(Int3);
				TYPE_CASE(Int4);
				TYPE_CASE(Int2x2);
				TYPE_CASE(Int2x3);
				TYPE_CASE(Int2x4);
				TYPE_CASE(Int3x2);
				TYPE_CASE(Int3x3);
				TYPE_CASE(Int3x4);
				TYPE_CASE(Int4x2);
				TYPE_CASE(Int4x3);
				TYPE_CASE(Int4x4);
			default: return "Invalid";
			}
#undef TYPE_CASE
		}

		EType m_Type = EType::Invalid;
		uint32 m_Offset = 0;
		std::string m_Name;
	};

	struct ShaderConstantBlock
	{
		std::string m_Name;
		uint32 m_Size = 0;
		uint32 m_NumMembers = 0;
		std::unique_ptr<ShaderConstant[]> m_Members;

		void Swap(ShaderConstantBlock& other) noexcept
		{
			m_Name.swap(other.m_Name);
			std::swap(m_Size, other.m_Size);
			std::swap(m_NumMembers, other.m_NumMembers);
			m_Members.swap(other.m_Members);
		}
	};

	struct ShaderInfo
	{
		uint32 m_NumSamplers = 0;
		uint32 m_NumStorageTextures = 0;
		uint32 m_NumStorageBuffers = 0;
		uint32 m_NumUniformBuffers = 0;
		const char* m_EntryPoint = "main";
		EShaderStage m_Stage = EShaderStage::Invalid;

		ShaderConstantBlock m_Blocks[4];
	};
} // namespace apollo::rdr