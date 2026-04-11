#include <catch2/catch_test_macros.hpp>
#include <core/Assert.hpp>
#include <rendering/ShaderInfo.hpp>
#include <slang-com-ptr.h>

#define SLANG_TEST(name) TEST_CASE(name, "[slang][shaders]")

namespace {
	static constexpr const char* g_Test1SlangModule = TEST_DATA_DIR "/test1.slang";
	static constexpr const char* g_Test2SlangModule = TEST_DATA_DIR "/test2.slang";

	std::string_view ToStringView(slang::IBlob& blob)
	{
		return { static_cast<const char*>(blob.getBufferPointer()), blob.getBufferSize() };
	}

	struct Compiler
	{
		template <class T>
		using Ref = Slang::ComPtr<T>;

		Ref<slang::IGlobalSession> m_GlobalSession;
		Ref<slang::ISession> m_Session;

		Compiler()
		{
			slang::createGlobalSession(m_GlobalSession.writeRef());
			const slang::TargetDesc targetDesc{
				.format = SLANG_SPIRV,
				.profile = m_GlobalSession->findProfile("spirv_1_3"),
			};
			const slang::SessionDesc sessionDesc{
				.targets = &targetDesc,
				.targetCount = 1,
				.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
			};
			m_GlobalSession->createSession(sessionDesc, m_Session.writeRef());
			APOLLO_ASSERT(m_Session, "Failed to create session");
		}

		slang::IModule* Compile(const char* name)
		{
			Ref<slang::IBlob> diagnostics;
			auto* module = m_Session->loadModule(name, diagnostics.writeRef());
			if (!module && diagnostics)
			{
				APOLLO_LOG_ERROR("Failed to load module {}:\n{}", name, ToStringView(*diagnostics));
			}
			return module;
		}
		Ref<slang::IComponentType> Link(
			slang::IModule& module,
			const char* entryPoint = "main",
			SlangStage stage = SlangStage::SLANG_STAGE_NONE,
			slang::IMetadata** out_metadata = nullptr)
		{
			Ref<slang::IComponentType> composite, linkedPgm;
			Ref<slang::IEntryPoint> ep;
			Ref<slang::IBlob> diagnostics;
			if (stage == SLANG_STAGE_NONE)
			{
				module.findEntryPointByName(entryPoint, ep.writeRef());
			}
			else
			{
				module.findAndCheckEntryPoint(
					entryPoint,
					stage,
					ep.writeRef(),
					diagnostics.writeRef());
			}

			if (!ep)
			{
				APOLLO_LOG_ERROR("Failed to find entry point {}", entryPoint);
				return nullptr;
			}
			slang::IComponentType* const components[]{ &module, ep };
			m_Session->createCompositeComponentType(
				components,
				2,
				composite.writeRef(),
				diagnostics.writeRef());
			if (!composite)
			{
				APOLLO_LOG_ERROR("Failed to compose program:\n{}", ToStringView(*diagnostics));
				return linkedPgm;
			}

			composite->link(linkedPgm.writeRef(), diagnostics.writeRef());
			if (!linkedPgm)
			{
				APOLLO_LOG_ERROR("Failed to link program:\n{}", ToStringView(*diagnostics));
			}
			else if (out_metadata)
			{
				linkedPgm->getEntryPointMetadata(0, 0, out_metadata);
			}
			return linkedPgm;
		}

		Ref<slang::IComponentType> CompileAndLink(
			const char* name,
			const char* entryPoint = "main",
			SlangStage stage = SLANG_STAGE_NONE,
			slang::IMetadata** out_metadata = nullptr)
		{
			auto* module = Compile(name);
			if (!module)
				return nullptr;

			return Link(*module, entryPoint, stage, out_metadata);
		}
	};
} // namespace

namespace apollo::rdr::shader_ut {
	using Slang::ComPtr;

	SLANG_TEST("Enumerate Descriptors")
	{
		Compiler compiler;
		auto* module = compiler.Compile(g_Test1SlangModule);
		REQUIRE(module);
		SECTION("Vertex Shader")
		{
			ComPtr<slang::IMetadata> metadata;
			ComPtr program = compiler.Link(
				*module,
				"vs_main",
				SLANG_STAGE_VERTEX,
				metadata.writeRef());
			REQUIRE(program);
			REQUIRE(metadata);

			ShaderInfo info = ShaderInfo::FromSlangModule(
				*program,
				rdr::EShaderStage::Vertex,
				metadata);

			CHECK(info.m_NumSamplers == 0);
			CHECK(info.m_NumStorageTextures == 0);
			CHECK(info.m_NumStorageBuffers == 1);
			CHECK(info.m_NumUniformBuffers == 1);

			const auto& block = info.m_Blocks[0];
			CHECK(block.m_Name == "g_Camera");
			CHECK(block.m_Size == 64);
			CHECK(block.m_NumMembers == 1);
			CHECK(block.m_Members[0].m_Name == "g_Camera");
			CHECK(block.m_Members[0].m_Type == ShaderConstant::EType::Float4x4);
			CHECK(block.m_Members[0].m_Offset == 0);
		}
		SECTION("Fragment Shader")
		{
			ComPtr<slang::IMetadata> metadata;
			ComPtr program = compiler.Link(
				*module,
				"fs_main",
				SLANG_STAGE_FRAGMENT,
				metadata.writeRef());
			REQUIRE(program);
			REQUIRE(metadata);

			ShaderInfo info = ShaderInfo::FromSlangModule(
				*program,
				rdr::EShaderStage::Fragment,
				metadata);
			CHECK(info.m_NumSamplers == 2);
			CHECK(info.m_NumStorageBuffers == 0);
			CHECK(info.m_NumStorageTextures == 0);
			CHECK(info.m_NumUniformBuffers == 0);
		}
	}
	SLANG_TEST("Enumerate Constants")
	{
		Compiler compiler;
		auto* module = compiler.Compile(g_Test2SlangModule);
		REQUIRE(module);
		SECTION("Vertex Shader")
		{
			ComPtr<slang::IMetadata> metadata;
			ComPtr program = compiler.Link(
				*module,
				"vs_main",
				SLANG_STAGE_VERTEX,
				metadata.writeRef());
			REQUIRE(program);
			REQUIRE(metadata);

			ShaderInfo info = ShaderInfo::FromSlangModule(
				*program,
				rdr::EShaderStage::Vertex,
				metadata);

			CHECK(info.m_NumSamplers == 0);
			CHECK(info.m_NumStorageTextures == 0);
			CHECK(info.m_NumStorageBuffers == 0);
			CHECK(info.m_NumUniformBuffers == 2);

			{
				const auto& block = info.m_Blocks[0];
				CHECK(block.m_Name == "Camera");
				CHECK(block.m_Size == 64);
				CHECK(block.m_NumMembers == 1);
				CHECK(block.m_Members[0].m_Name == "Camera");
				CHECK(block.m_Members[0].m_Type == ShaderConstant::EType::Float4x4);
				CHECK(block.m_Members[0].m_Offset == 0);
			}
			{
				const auto& block = info.m_Blocks[1];
				CHECK(block.m_Name == "Transform");
				CHECK(block.m_Size == 80);
				CHECK(block.m_NumMembers == 2);

				CHECK(block.m_Members[0].m_Name == "Base");
				CHECK(block.m_Members[0].m_Type == ShaderConstant::EType::Float4x4);
				CHECK(block.m_Members[0].m_Offset == 0);

				CHECK(block.m_Members[1].m_Name == "Offset");
				CHECK(block.m_Members[1].m_Type == ShaderConstant::EType::Float4);
				CHECK(block.m_Members[1].m_Offset == 64);
			}
		}
	}
} // namespace apollo::rdr::shader_ut