#include "ShaderInfo.hpp"
#include <core/Assert.hpp>
#include <core/Enum.hpp>
#include <slang.h>

namespace {
	struct Binding
	{
		size_t space = 0, index = 0;

		Binding GetDescriptorBinding(slang::VariableLayoutReflection& var) const
		{
			return Binding{
				.space = space + var.getBindingSpace(slang::DescriptorTableSlot),
				.index = index + var.getOffset(slang::DescriptorTableSlot),
			};
		}
		Binding GetParamBlockBinding(slang::VariableLayoutReflection& var) const
		{
			return Binding{
				.space = space + var.getOffset(slang::SubElementRegisterSpace),
				.index = 0,
			};
		}
	};
	apollo::rdr::ShaderConstant::EType GetScalarType(slang::TypeReflection::ScalarType t)
	{
		using EType = apollo::rdr::ShaderConstant::EType;
		switch (t)
		{
		case slang::TypeReflection::ScalarType::Float32: return EType::Float;
		case slang::TypeReflection::ScalarType::Int32: return EType::Int;
		case slang::TypeReflection::ScalarType::UInt32: return EType::UInt;
		default: return EType::Invalid;
		}
	}

	apollo::rdr::ShaderConstant::EType GetVectorType(
		slang::TypeReflection::ScalarType elemType,
		uint32 count = 1)
	{
		using namespace apollo::enum_operators;
		using EType = apollo::rdr::ShaderConstant::EType;
		const auto baseType = GetScalarType(elemType);
		return baseType == EType::Invalid ? EType::Invalid : baseType + (count - 1);
	}

	apollo::rdr::ShaderConstant::EType GetMatrixType(
		slang::TypeReflection::ScalarType elemType,
		uint32 rows,
		uint32 cols)
	{
		using namespace apollo::enum_operators;
		const auto baseType = GetVectorType(elemType, cols);
		if (baseType == decltype(baseType)::Invalid)
			return baseType;
		return baseType + (3 * (rows - 1));
	}
	apollo::rdr::ShaderConstant CreateConstant(
		slang::TypeLayoutReflection& type,
		const char* name,
		uint32 offset = 0)
	{
		apollo::rdr::ShaderConstant res{
			.m_Offset = offset,
			.m_Name = name,
		};
		switch (type.getKind())
		{
		case slang::TypeReflection::Kind::Scalar:
			res.m_Type = GetScalarType(type.getScalarType());
			break;
		case slang::TypeReflection::Kind::Vector:
			res.m_Type = GetVectorType(
				type.getElementTypeLayout()->getScalarType(),
				type.getElementCount());
			break;
		case slang::TypeReflection::Kind::Matrix:
			res.m_Type = GetMatrixType(
				type.getElementTypeLayout()->getScalarType(),
				type.getRowCount(),
				type.getColumnCount());
			break;
		default: break;
		}
		return res;
	}

	struct ReflectionContext
	{
		using ShaderInfo = apollo::rdr::ShaderInfo;
		ShaderInfo& m_Info;
		slang::IMetadata* m_Metadata = nullptr;
		uint32 m_SetIndex;
		const char* m_StageName;

		ReflectionContext(
			ShaderInfo& out_info,
			slang::IComponentType& module,
			apollo::rdr::EShaderStage stage,
			slang::IMetadata* metadata = nullptr)
			: m_Info(out_info)
			, m_Metadata(metadata)
		{
			switch (stage)
			{
			case apollo::rdr::EShaderStage::Vertex:
				m_SetIndex = 0;
				m_StageName = "vertex";
				break;
			case apollo::rdr::EShaderStage::Fragment:
				m_SetIndex = 2;
				m_StageName = "fragment";
				break;
			default: APOLLO_LOG_CRITICAL("Invalid shader stage {}", int32(stage)); DEBUG_BREAK();
			}
			TraverseScope(module.getLayout()->getGlobalParamsTypeLayout());
		}

	private:
		enum EResourceUsage : int8
		{
			Ignored,
			Ok,
			Invalid,
		};

		static bool IsDescriptorUsed(Binding binding, slang::IMetadata* metadata)
		{
			bool used = true;
			if (metadata)
			{
				metadata->isParameterLocationUsed(
					SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT,
					binding.space,
					binding.index,
					used);
			}
			return used;
		}

		void TraverseScope(slang::TypeLayoutReflection* scope, Binding baseBinding = {})
		{
			using TypeKind = slang::TypeReflection::Kind;

			for (uint32 i = 0; i < scope->getFieldCount(); ++i)
			{
				auto* field = scope->getFieldByIndex(i);
				auto* type = field->getTypeLayout();
				const auto kind = type->getKind();

				switch (kind)
				{
				case TypeKind::ParameterBlock:
				{
					const Binding b{ baseBinding.GetParamBlockBinding(*field) };
					if (b.space != m_SetIndex && b.space != (m_SetIndex + 1))
						continue;
					TraverseScope(type->getElementTypeLayout(), b);
				}
				break;
				case TypeKind::Struct: TraverseScope(type, baseBinding); break;

				case TypeKind::ConstantBuffer:
				{
					const Binding binding = baseBinding.GetDescriptorBinding(*field);
					HandleConstantBuffer(*field, *type->getElementTypeLayout(), binding);
				}
				break;
				case TypeKind::Resource:
				{
					const Binding binding = baseBinding.GetDescriptorBinding(*field);
					HandleResource(field->getName(), type->getResourceShape(), binding);
				}

				default: break;
				}
			}
		}
		/**
		 * Checks whether a desource descriptor is used and if yes, whether it is declared in the
		 * correct set
		 */
		EResourceUsage CheckDescriptorAccess(const char* name, Binding binding, uint32_t expectedSet)
		{
			if (binding.space == expectedSet)
				return EResourceUsage::Ok;

			if (!IsDescriptorUsed(binding, m_Metadata) &&
				((binding.space & 1) == (expectedSet & 1)))
			{
				return EResourceUsage::Ignored;
			}

			APOLLO_LOG_ERROR(
				"Descriptor {} is declared in set {}, which is invalid for a {} "
				"shader. This descriptor must be declared in set {} (see SDL3 docs for "
				"details)",
				name,
				binding.space,
				m_StageName,
				expectedSet);
			return EResourceUsage::Invalid;
		}

		void HandleConstantBuffer(
			slang::VariableLayoutReflection& var,
			slang::TypeLayoutReflection& type,
			Binding binding)
		{
			if (m_Info.m_NumUniformBuffers >= STATIC_ARRAY_SIZE(m_Info.m_Blocks)) [[unlikely]]
				return;

			if (CheckDescriptorAccess(var.getName(), binding, m_SetIndex + 1) != EResourceUsage::Ok)
				return;

			auto& block = m_Info.m_Blocks[m_Info.m_NumUniformBuffers++];
			block.m_Name = var.getName();
			block.m_Size = type.getSize();
			block.m_NumMembers = type.getFieldCount();
			if (!block.m_NumMembers)
			{
				auto constant = CreateConstant(type, var.getName());
				if (constant.m_Type != apollo::rdr::ShaderConstant::EType::Invalid)
				{
					block.m_NumMembers = 1;
					block.m_Members = std::make_unique<decltype(constant)[]>(1);
					block.m_Members[0] = std::move(constant);
				}
				return;
			}

			block.m_Members = std::make_unique<apollo::rdr::ShaderConstant[]>(block.m_NumMembers);
			for (uint32 i = 0; i < block.m_NumMembers; ++i)
			{
				auto* field = type.getFieldByIndex(i);
				auto* fieldType = field->getTypeLayout();
				block.m_Members[i] = CreateConstant(
					*fieldType,
					field->getName(),
					field->getOffset());
				TraverseScope(fieldType, binding);
			}
		}

		void HandleResource(const char* name, SlangResourceShape shape, Binding binding)
		{
			if (CheckDescriptorAccess(name, binding, m_SetIndex) != EResourceUsage::Ok)
				return;

			switch (shape & SLANG_RESOURCE_BASE_SHAPE_MASK)
			{
			case SLANG_TEXTURE_1D:
			case SLANG_TEXTURE_2D:
			case SLANG_TEXTURE_3D:
			case SLANG_TEXTURE_CUBE:
				if (shape & SLANG_TEXTURE_COMBINED_FLAG)
				{
					++m_Info.m_NumSamplers;
				}
				break;
			case SLANG_TEXTURE_BUFFER: ++m_Info.m_NumStorageTextures; break;
			case SLANG_STRUCTURED_BUFFER: ++m_Info.m_NumStorageBuffers; break;
			}
		}
	};
} // namespace

namespace apollo::rdr {
	ShaderInfo ShaderInfo::FromSlangModule(
		slang::IComponentType& mod,
		EShaderStage stage,
		slang::IMetadata* metadata)
	{
		ShaderInfo info{ .m_Stage = stage };
		ReflectionContext ctx{ info, mod, stage, metadata };
		return info;
	}
} // namespace apollo::rdr