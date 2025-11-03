#include <SDL3/SDL_gpu.h>
#include <catch2/catch_test_macros.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <core/Window.hpp>
#include <rendering/Pipeline.hpp>
#include <rendering/Context.hpp>

#define PIPELINE_TEST(name) TEST_CASE(name, "[pipeline][rdr]")

namespace apollo::rdr {
	using RenderContext = Context;

	struct TestContext
	{
		TestContext()
			: m_Window{
				WindowSettings{
					.m_Title = "Apollo Rendering Tests",
					.m_Width = 100,
					.m_Height = 100,
					.m_Hidden = true,
				},
			}
		{
			m_Device = &RenderContext::Init(EBackend::Vulkan, m_Window).GetDevice();
		}

		~TestContext() { RenderContext::Shutdown(); }

		Window m_Window;
		rdr::GPUDevice* m_Device = nullptr;
	};

	PIPELINE_TEST("Load Default Pipeline description from JSON")
	{
		TestContext ctx;
		using ConverterType = apollo::json::Converter<SDL_GPUGraphicsPipelineCreateInfo>;
		const nlohmann::json j{};
		SDL_GPUGraphicsPipelineCreateInfo info = {};

		CHECK(ConverterType::FromJson(info, j));
		CHECK(info.vertex_input_state.num_vertex_attributes == 0);
		CHECK(info.vertex_input_state.num_vertex_buffers == 0);
		CHECK(info.vertex_input_state.vertex_attributes == nullptr);
		CHECK(info.vertex_input_state.vertex_buffer_descriptions == nullptr);
		CHECK(info.primitive_type == SDL_GPU_PRIMITIVETYPE_TRIANGLELIST);

		CHECK(info.rasterizer_state.fill_mode == SDL_GPU_FILLMODE_FILL);
		CHECK(info.rasterizer_state.cull_mode == SDL_GPU_CULLMODE_NONE);
		CHECK(info.rasterizer_state.front_face == SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE);
		CHECK(info.rasterizer_state.depth_bias_clamp == 0);
		CHECK(info.rasterizer_state.depth_bias_constant_factor == 0);
		CHECK(info.rasterizer_state.depth_bias_slope_factor == 0);
		CHECK_FALSE(info.rasterizer_state.enable_depth_bias);
		CHECK_FALSE(info.rasterizer_state.enable_depth_clip);

		CHECK_FALSE(info.depth_stencil_state.enable_depth_test);
		CHECK_FALSE(info.depth_stencil_state.enable_stencil_test);
		CHECK_FALSE(info.depth_stencil_state.enable_depth_write);

		CHECK_FALSE(info.target_info.has_depth_stencil_target);
		CHECK(info.target_info.num_color_targets == 1);

		const auto& colorTarget = info.target_info.color_target_descriptions[0];
		CHECK(
			colorTarget.format ==
			SDL_GetGPUSwapchainTextureFormat(ctx.m_Device->GetHandle(), ctx.m_Window.GetHandle()));

		CHECK_FALSE(colorTarget.blend_state.enable_blend);
		CHECK_FALSE(colorTarget.blend_state.enable_color_write_mask);
	}

	PIPELINE_TEST("Load Custom Pipeline description from JSON")
	{
		TestContext ctx;

		using ConverterType = apollo::json::Converter<SDL_GPUGraphicsPipelineCreateInfo>;
		const nlohmann::json j = nlohmann::json::parse(R"json({
	"input": "vertex2d",
	"primitiveType": "pointList",
	"rasterizer": {
		"fillMode": "line",
		"cullMode": "back",
		"depthBiasClamp": 1,
		"depthBiasConstantFactor": 1,
		"depthBiasSlopeFactor": 1,
		"enableDepthBias": true,
		"enableDepthClip": true
	},
	"depthStencil": {
		"enableDepthTest": true,
		"enableStencilTest": true,
		"enableDepthWrite": true,
		"compareMask": 1,
		"writeMask": 2,
		"compareOp": "lessOrEqual",
		"frontStencil": {
			"compareOp": "never",
			"passOp": "incrementAndWrap",
			"failOp": "invert",
			"depthFailOp": "replace"
		},
		"backStencil": {
			"compareOp": "never",
			"passOp": "incrementAndWrap",
			"failOp": "invert",
			"depthFailOp": "replace"
		},
		"format": "d24unorm_s8uint"
	},
	"colorTargets": [
		{
			"format": "rgba8unorm",
			"blending": {
				"srcColorBlendFactor": "srcAlpha",
				"dstColorBlendFactor": "oneMinusSrcAlpha",
				"colorBlendOp": "add",
				"srcAlphaBlendFactor": "one",
				"dstAlphaBlendFactor": "zero",
				"alphaBlendOp": "subtract",
				"colorWriteMask": 15
			}
		},
		{
			"format": "rgba8unorm_srgb"
		}
	]
})json");

		REQUIRE(!j["primitiveType"].is_object());

		SDL_GPUGraphicsPipelineCreateInfo info = {};
		CHECK(ConverterType::FromJson(info, j));

		const auto& inputState = GetStandardVertexInputState(EStandardVertexType::Vertex2d);
		CHECK(info.vertex_input_state.num_vertex_buffers == 1);
		CHECK(info.vertex_input_state.num_vertex_attributes == 2);
		CHECK(
			info.vertex_input_state.vertex_buffer_descriptions ==
			inputState.vertex_buffer_descriptions);
		CHECK(info.vertex_input_state.vertex_attributes == inputState.vertex_attributes);
		CHECK(info.primitive_type == SDL_GPU_PRIMITIVETYPE_POINTLIST);

		CHECK(info.rasterizer_state.fill_mode == SDL_GPU_FILLMODE_LINE);
		CHECK(info.rasterizer_state.cull_mode == SDL_GPU_CULLMODE_BACK);
		CHECK(info.rasterizer_state.depth_bias_clamp == 1);
		CHECK(info.rasterizer_state.depth_bias_constant_factor == 1);
		CHECK(info.rasterizer_state.depth_bias_slope_factor == 1);
		CHECK(info.rasterizer_state.enable_depth_bias);
		CHECK(info.rasterizer_state.enable_depth_clip);

		CHECK(info.depth_stencil_state.enable_depth_test);
		CHECK(info.depth_stencil_state.enable_depth_write);
		CHECK(info.depth_stencil_state.enable_stencil_test);
		CHECK(info.depth_stencil_state.compare_mask == 1);
		CHECK(info.depth_stencil_state.write_mask == 2);
		CHECK(info.depth_stencil_state.compare_op == SDL_GPU_COMPAREOP_LESS_OR_EQUAL);

		CHECK(info.depth_stencil_state.front_stencil_state.compare_op == SDL_GPU_COMPAREOP_NEVER);
		CHECK(
			info.depth_stencil_state.front_stencil_state.pass_op ==
			SDL_GPU_STENCILOP_INCREMENT_AND_WRAP);
		CHECK(info.depth_stencil_state.front_stencil_state.fail_op == SDL_GPU_STENCILOP_INVERT);
		CHECK(
			info.depth_stencil_state.front_stencil_state.depth_fail_op ==
			SDL_GPU_STENCILOP_REPLACE);

		CHECK(info.depth_stencil_state.back_stencil_state.compare_op == SDL_GPU_COMPAREOP_NEVER);
		CHECK(
			info.depth_stencil_state.back_stencil_state.pass_op ==
			SDL_GPU_STENCILOP_INCREMENT_AND_WRAP);
		CHECK(info.depth_stencil_state.back_stencil_state.fail_op == SDL_GPU_STENCILOP_INVERT);
		CHECK(
			info.depth_stencil_state.back_stencil_state.depth_fail_op == SDL_GPU_STENCILOP_REPLACE);

		CHECK(info.target_info.has_depth_stencil_target);
		CHECK(info.target_info.depth_stencil_format == SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT);
		CHECK(info.target_info.num_color_targets == 2);

		const auto* colorTarget = info.target_info.color_target_descriptions;
		CHECK(colorTarget->format == SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);

		CHECK(colorTarget->blend_state.enable_blend);
		CHECK(colorTarget->blend_state.src_color_blendfactor == SDL_GPU_BLENDFACTOR_SRC_ALPHA);
		CHECK(
			colorTarget->blend_state.dst_color_blendfactor ==
			SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
		CHECK(colorTarget->blend_state.color_blend_op == SDL_GPU_BLENDOP_ADD);
		CHECK(colorTarget->blend_state.src_alpha_blendfactor == SDL_GPU_BLENDFACTOR_ONE);
		CHECK(colorTarget->blend_state.dst_alpha_blendfactor == SDL_GPU_BLENDFACTOR_ZERO);
		CHECK(colorTarget->blend_state.alpha_blend_op == SDL_GPU_BLENDOP_SUBTRACT);
		CHECK(colorTarget->blend_state.enable_color_write_mask);
		CHECK(colorTarget->blend_state.color_write_mask == 15);

		++colorTarget;
		CHECK(colorTarget->format == SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB);
		CHECK_FALSE(colorTarget->blend_state.enable_blend);
		CHECK_FALSE(colorTarget->blend_state.enable_color_write_mask);
		CHECK(colorTarget->blend_state.src_color_blendfactor == SDL_GPU_BLENDFACTOR_ONE);
		CHECK(colorTarget->blend_state.dst_color_blendfactor == SDL_GPU_BLENDFACTOR_ZERO);
		CHECK(colorTarget->blend_state.color_blend_op == SDL_GPU_BLENDOP_ADD);
		CHECK(colorTarget->blend_state.src_alpha_blendfactor == SDL_GPU_BLENDFACTOR_ONE);
		CHECK(colorTarget->blend_state.dst_alpha_blendfactor == SDL_GPU_BLENDFACTOR_ZERO);
		CHECK(colorTarget->blend_state.alpha_blend_op == SDL_GPU_BLENDOP_ADD);
	}

} // namespace apollo::rdr