#include "Pipeline.hpp"
#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <core/Window.hpp>

namespace {
	thread_local SDL_GPUColorTargetDescription g_ColorTargets[apollo::rdr::MaxColorTargets];
}

namespace apollo::rdr {
	NLOHMANN_JSON_SERIALIZE_ENUM(
		EStandardVertexType,
		{
			{ EStandardVertexType::Invalid, nullptr },
			{ EStandardVertexType::Vertex2d, "vertex2d" },
			{ EStandardVertexType::Vertex3d, "vertex3d" },
		});
} // namespace apollo::rdr

NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUPrimitiveType,
	{
		{ (SDL_GPUPrimitiveType)-1, nullptr },
		{ SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, "triangleList" },
		{ SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP, "triangleStrip" },
		{ SDL_GPU_PRIMITIVETYPE_LINELIST, "lineList" },
		{ SDL_GPU_PRIMITIVETYPE_LINESTRIP, "lineStrip" },
		{ SDL_GPU_PRIMITIVETYPE_POINTLIST, "pointList" },
	});

NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUTextureFormat,
	{
		{ SDL_GPU_TEXTUREFORMAT_INVALID, nullptr },

		{ SDL_GPU_TEXTUREFORMAT_R8_UNORM, "r8unorm" },
		{ SDL_GPU_TEXTUREFORMAT_R8G8_UNORM, "rg8unorm" },
		{ SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, "rgba8unorm" },
		{ SDL_GPU_TEXTUREFORMAT_R16_UNORM, "r16usorm" },
		{ SDL_GPU_TEXTUREFORMAT_R16G16_UNORM, "rg16unorm" },
		{ SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM, "rgba16unorm" },

		{ SDL_GPU_TEXTUREFORMAT_R8_SNORM, "r8usorm" },
		{ SDL_GPU_TEXTUREFORMAT_R8G8_SNORM, "rg8snorm" },
		{ SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM, "rgba8unorm" },
		{ SDL_GPU_TEXTUREFORMAT_R16_SNORM, "r16ssorm" },
		{ SDL_GPU_TEXTUREFORMAT_R16G16_SNORM, "rg16snorm" },
		{ SDL_GPU_TEXTUREFORMAT_R16G16B16A16_SNORM, "rgba16snorm" },

		{ SDL_GPU_TEXTUREFORMAT_R8_INT, "r8int" },
		{ SDL_GPU_TEXTUREFORMAT_R8G8_INT, "rg8int" },
		{ SDL_GPU_TEXTUREFORMAT_R8G8B8A8_INT, "rgba8int" },

		{ SDL_GPU_TEXTUREFORMAT_R8_UINT, "r8uint" },
		{ SDL_GPU_TEXTUREFORMAT_R8G8_UINT, "rg8uint" },
		{ SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT, "rgba8uint" },

		{ SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB, "rgba8unorm_srgb" },
		{ SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB, "bgra8unorm_srgb" },

		{ SDL_GPU_TEXTUREFORMAT_D16_UNORM, "d16unorm" },
		{ SDL_GPU_TEXTUREFORMAT_D24_UNORM, "d24unorm" },
		{ SDL_GPU_TEXTUREFORMAT_D32_FLOAT, "d32float" },
		{ SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT, "d24unorm_s8uint" },
		{ SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT, "d32float_s8uint" },
	})

NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUCullMode,
	{
		{ (SDL_GPUCullMode)-1, nullptr },
		{ SDL_GPU_CULLMODE_NONE, "none" },
		{ SDL_GPU_CULLMODE_BACK, "back" },
		{ SDL_GPU_CULLMODE_FRONT, "front" },
	});
NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUFillMode,
	{
		{ (SDL_GPUFillMode)-1, nullptr },
		{ SDL_GPU_FILLMODE_FILL, "fill" },
		{ SDL_GPU_FILLMODE_LINE, "line" },
	});

NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUSampleCount,
	{
		{ (SDL_GPUSampleCount)-1, nullptr },
		{ SDL_GPU_SAMPLECOUNT_1, 1 },
		{ SDL_GPU_SAMPLECOUNT_2, 2 },
		{ SDL_GPU_SAMPLECOUNT_4, 4 },
		{ SDL_GPU_SAMPLECOUNT_8, 8 },
	});
NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUCompareOp,
	{
		{ SDL_GPU_COMPAREOP_INVALID, nullptr },
		{ SDL_GPU_COMPAREOP_ALWAYS, "always" },
		{ SDL_GPU_COMPAREOP_NEVER, "never" },
		{ SDL_GPU_COMPAREOP_EQUAL, "equal" },
		{ SDL_GPU_COMPAREOP_NOT_EQUAL, "notEqual" },
		{ SDL_GPU_COMPAREOP_LESS, "less" },
		{ SDL_GPU_COMPAREOP_LESS_OR_EQUAL, "lessOrEqual" },
		{ SDL_GPU_COMPAREOP_GREATER, "greater" },
		{ SDL_GPU_COMPAREOP_GREATER_OR_EQUAL, "greaterOrEqual" },
	});
NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUStencilOp,
	{
		{ SDL_GPU_STENCILOP_INVALID, nullptr },
		{ SDL_GPU_STENCILOP_ZERO, "zero" },
		{ SDL_GPU_STENCILOP_REPLACE, "replace" },
		{ SDL_GPU_STENCILOP_INVERT, "invert" },
		{ SDL_GPU_STENCILOP_DECREMENT_AND_WRAP, "decrementAndWrap" },
		{ SDL_GPU_STENCILOP_DECREMENT_AND_CLAMP, "decrementAndClamp" },
		{ SDL_GPU_STENCILOP_INCREMENT_AND_WRAP, "incrementAndWrap" },
		{ SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP, "incrementAndClamp" },
	});
NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUBlendFactor,
	{
		{ SDL_GPU_BLENDFACTOR_INVALID, nullptr },
		{ SDL_GPU_BLENDFACTOR_ZERO, "zero" },
		{ SDL_GPU_BLENDFACTOR_ONE, "one" },
		{ SDL_GPU_BLENDFACTOR_SRC_COLOR, "srcColor" },
		{ SDL_GPU_BLENDFACTOR_SRC_ALPHA, "srcAlpha" },
		{ SDL_GPU_BLENDFACTOR_DST_COLOR, "dstColor" },
		{ SDL_GPU_BLENDFACTOR_DST_ALPHA, "dstAlpha" },
		{ SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA, "oneMinusDstAlpha" },
		{ SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, "oneMinusSrcAlpha" },
		{ SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR, "oneMinusSrcColor" },
		{ SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR, "oneMinusDstColor" },
		{ SDL_GPU_BLENDFACTOR_CONSTANT_COLOR, "constantColor" },
		{ SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR, "oneMinusConstantColor" },
		{ SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE, "srcAlphaSaturate" },
	});

NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUBlendOp,
	{
		{ SDL_GPU_BLENDOP_INVALID, nullptr },
		{ SDL_GPU_BLENDOP_ADD, "add" },
		{ SDL_GPU_BLENDOP_SUBTRACT, "subtract" },
		{ SDL_GPU_BLENDOP_REVERSE_SUBTRACT, "reverseSubtract" },
		{ SDL_GPU_BLENDOP_MIN, "min" },
		{ SDL_GPU_BLENDOP_MAX, "max" },
	});

template <>
struct apollo::json::Converter<SDL_GPUVertexInputState>
{
	static bool FromJson(SDL_GPUVertexInputState& out_state, const nlohmann::json& json) noexcept
	{
		rdr::EStandardVertexType type;
		json.get_to(type);
		if (type == rdr::EStandardVertexType::Invalid)
		{
			return false;
		}
		out_state = rdr::GetStandardVertexInputState(type);
		return true;
	}
};

template <>
struct apollo::json::Converter<SDL_GPUColorTargetBlendState>
{
	static bool FromJson(SDL_GPUColorTargetBlendState& out_state, const nlohmann::json& json) noexcept
	{
		if (!Visit(out_state.enable_blend, json, "enableBlend", true))
			return false;

		if (!Visit(out_state.enable_color_write_mask, json, "enableWriteMask", true))
			return false;

		if (out_state.enable_color_write_mask &&
			!Visit(out_state.color_write_mask, json, "colorWriteMask"))
			return false;

		if (!out_state.enable_blend)
			return true;

		out_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		out_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
		out_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		out_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		out_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
		out_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

		if (!Visit(out_state.src_color_blendfactor, json, "srcColorBlendFactor", true) ||
			out_state.src_color_blendfactor == SDL_GPU_BLENDFACTOR_INVALID)
			return false;

		if (!Visit(out_state.dst_color_blendfactor, json, "dstColorBlendFactor", true) ||
			out_state.dst_color_blendfactor == SDL_GPU_BLENDFACTOR_INVALID)
			return false;

		if (!Visit(out_state.src_alpha_blendfactor, json, "srcAlphaBlendFactor", true) ||
			out_state.src_alpha_blendfactor == SDL_GPU_BLENDFACTOR_INVALID)
			return false;

		if (!Visit(out_state.dst_alpha_blendfactor, json, "dstAlphaBlendFactor", true) ||
			out_state.dst_alpha_blendfactor == SDL_GPU_BLENDFACTOR_INVALID)
			return false;

		if (!Visit(out_state.color_blend_op, json, "colorBlendOp", true) ||
			out_state.color_blend_op == SDL_GPU_BLENDOP_INVALID)
			return false;

		return Visit(out_state.alpha_blend_op, json, "alphaBlendOp", true) &&
			   out_state.alpha_blend_op != SDL_GPU_BLENDOP_INVALID;
	}
};

template <>
struct apollo::json::Converter<SDL_GPUColorTargetDescription>
{
	static bool FromJson(SDL_GPUColorTargetDescription& out_desc, const nlohmann::json& json) noexcept
	{
		auto it = json.find("format");
		if (it != json.end())
		{
			if (!it->is_string())
				return false;
			it->get_to(out_desc.format);
			if (out_desc.format == SDL_GPU_TEXTUREFORMAT_INVALID)
				return false;
		}
		else
		{
			auto* renderer = rdr::Renderer::GetInstance();
			auto& window = renderer->GetWindow();
			auto& device = renderer->GetDevice();
			out_desc.format = SDL_GetGPUSwapchainTextureFormat(
				device.GetHandle(),
				window.GetHandle());
		}
		out_desc.blend_state = SDL_GPUColorTargetBlendState{
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.color_write_mask = 0b1111,
			.enable_blend = false,
		};

		return Visit(out_desc.blend_state, json, "blending", true);
	}
};

template <>
struct apollo::json::Converter<SDL_GPURasterizerState>
{
	static bool FromJson(SDL_GPURasterizerState& out_state, const nlohmann::json& json) noexcept
	{
		out_state = SDL_GPURasterizerState{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
		};

		if (!Visit(out_state.fill_mode, json, "fillMode", true) ||
			out_state.fill_mode == (SDL_GPUFillMode)-1)
			return false;
		if (!Visit(out_state.cull_mode, json, "cullMode", true) ||
			out_state.cull_mode == (SDL_GPUCullMode)-1)
			return false;

		if (!Visit(out_state.depth_bias_clamp, json, "depthBiasClamp", true))
			return false;
		if (!Visit(out_state.depth_bias_constant_factor, json, "depthBiasConstantFactor", true))
			return false;
		if (!Visit(out_state.depth_bias_slope_factor, json, "depthBiasSlopeFactor", true))
			return false;
		if (!Visit(out_state.enable_depth_bias, json, "enableDepthBias", true))
			return false;

		return Visit(out_state.enable_depth_clip, json, "enableDepthClip", true);

		return true;
	}
};

template <>
struct apollo::json::Converter<SDL_GPUStencilOpState>
{
	static bool FromJson(SDL_GPUStencilOpState& out_state, const nlohmann::json& json) noexcept
	{
		if (!Visit(out_state.pass_op, json, "passOp") ||
			out_state.pass_op == SDL_GPU_STENCILOP_INVALID)
			return false;
		if (!Visit(out_state.fail_op, json, "failOp") ||
			out_state.fail_op == SDL_GPU_STENCILOP_INVALID)
			return false;
		if (!Visit(out_state.depth_fail_op, json, "depthFailOp") ||
			out_state.depth_fail_op == SDL_GPU_STENCILOP_INVALID)
			return false;

		return Visit(out_state.compare_op, json, "compareOp") &&
			   out_state.compare_op != SDL_GPU_COMPAREOP_INVALID;
	}
};

template <>
struct apollo::json::Converter<SDL_GPUDepthStencilState>
{
	static bool FromJson(SDL_GPUDepthStencilState& out_state, const nlohmann::json& json) noexcept
	{
		if (!Visit(out_state.enable_stencil_test, json, "enableStencilTest", true))
			return false;

		if (!Visit(out_state.enable_depth_test, json, "enableDepthTest", true))
			return false;

		if (out_state.enable_depth_test &&
			!Visit(out_state.enable_depth_write, json, "enableDepthWrite", true))
			return false;

		if (!Visit(out_state.compare_op, json, "compareOp", !out_state.enable_depth_test) ||
			out_state.compare_op == SDL_GPU_COMPAREOP_INVALID)
			return false;

		if (!Visit(
				out_state.front_stencil_state,
				json,
				"frontStencil",
				!out_state.enable_stencil_test))
			return false;
		if (!Visit(
				out_state.back_stencil_state,
				json,
				"backStencil",
				!out_state.enable_stencil_test))
			return false;

		if (!Visit(out_state.compare_mask, json, "compareMask", !out_state.enable_stencil_test))
			return false;

		return Visit(out_state.write_mask, json, "writeMask", !out_state.write_mask);
	}
};

namespace apollo::json {
	bool Converter<SDL_GPUGraphicsPipelineCreateInfo>::FromJson(
		SDL_GPUGraphicsPipelineCreateInfo& out_info,
		const nlohmann::json& json) noexcept
	{
		out_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

		if (!Visit(out_info.vertex_input_state, json, "input", true))
		{
			APOLLO_LOG_ERROR(
				"Failed to parse graphics pipeline description from JSON: invalid input state");
			return false;
		}

		if (!Visit(out_info.primitive_type, json, "primitiveType", true) ||
			out_info.primitive_type == (SDL_GPUPrimitiveType)-1)
		{
			APOLLO_LOG_ERROR(
				"Failed to parse graphics pipeline description from JSON: invalid primitive type");
			return false;
		}

		if (!Visit(out_info.rasterizer_state, json, "rasterizer", true))
		{
			APOLLO_LOG_ERROR(
				"Failed to parse graphics pipeline description from JSON: invalid rasterizer "
				"description");
			return false;
		}
		if (!Visit(out_info.multisample_state.sample_count, json, "sampleCount", true) ||
			out_info.multisample_state.sample_count == (SDL_GPUSampleCount)-1)
		{
			APOLLO_LOG_ERROR(
				"Failed to parse graphics pipeline description from JSON: invalid sample "
				"count");

			return false;
		}
		if (!Visit(out_info.depth_stencil_state, json, "depthStencil", true))
		{
			APOLLO_LOG_ERROR(
				"Failed to parse graphics pipeline description from JSON: invalid depth/stencil "
				"description");

			return false;
		}

		if (!Visit(
				out_info.target_info.has_depth_stencil_target,
				json,
				"hasDepthStencilTarget",
				true))
		{
			APOLLO_LOG_ERROR(
				"Failed to parse graphics pipeline description from JSON: invalid value for "
				"'hasDepthStencilTarget'");

			return false;
		}
		if (out_info.target_info.has_depth_stencil_target)
		{
			const bool res = Visit(
				out_info.target_info.depth_stencil_format,
				json,
				"depthStencilFormat");

			if (!res ||
				out_info.target_info.depth_stencil_format < SDL_GPU_TEXTUREFORMAT_D16_UNORM ||
				out_info.target_info.depth_stencil_format > SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT)
			{
				APOLLO_LOG_ERROR(
					"Failed to parse graphics pipeline description from JSON: missing/invalid "
					"depth/stencil format");
				return false;
			}
		}

		out_info.target_info.color_target_descriptions = g_ColorTargets;
		const auto it = json.find("colorTargets");
		if (it == json.end())
		{
			auto* renderer = rdr::Renderer::GetInstance();
			auto& window = renderer->GetWindow();
			auto& device = renderer->GetDevice();
			g_ColorTargets[0] = SDL_GPUColorTargetDescription{
				.format = SDL_GetGPUSwapchainTextureFormat(device.GetHandle(), window.GetHandle()),
				.blend_state =
					SDL_GPUColorTargetBlendState{
						.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
						.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
						.color_blend_op = SDL_GPU_BLENDOP_ADD,
						.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
						.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
						.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
						.color_write_mask = 0b1111,
						.enable_blend = false,
					},
			};
			out_info.target_info.num_color_targets = 1;
			return true;
		}
		if (!it->is_array())
		{
			APOLLO_LOG_ERROR(
				"Failed to parse graphics pipeline description from JSON: 'colorTargets' is not an "
				"array");
			return false;
		}
		if (it->size() > rdr::MaxColorTargets)
		{
			APOLLO_LOG_ERROR(
				"Failed to parse graphics pipeline description from JSON: too many color targets "
				"(maximum is {})",
				rdr::MaxColorTargets);
			return false;
		}

		uint32 targetIndex = 0;
		for (const nlohmann::json& targetJson : *it)
		{
			if (!Converter<SDL_GPUColorTargetDescription>::FromJson(
					g_ColorTargets[targetIndex],
					targetJson))
			{
				APOLLO_LOG_ERROR(
					"Failed to parse graphics pipeline description from JSON: colorTargets[{}] "
					"is not a valid color target description",
					targetIndex);
				return false;
			}
			++targetIndex;
		}
		out_info.target_info.num_color_targets = targetIndex;

		return true;
	}
} // namespace apollo::json