#pragma once

#include <PCH.hpp>

#include "VertexTypes.hpp"
#include <core/JsonFwd.hpp>

struct SDL_GPUGraphicsPipelineCreateInfo;

template <>
struct brk::json::Converter<SDL_GPUGraphicsPipelineCreateInfo>
{
	BRK_API static bool FromJson(
		SDL_GPUGraphicsPipelineCreateInfo& out_info,
		const nlohmann::json& json) noexcept;
	BRK_API static void ToJson(
		const SDL_GPUGraphicsPipelineCreateInfo& info,
		nlohmann::json& out_json) noexcept;
};

namespace brk::rdr {
	// Maximum number of color targets for a graphics pipeline output
	static constexpr uint32 MaxColorTargets = 8;
} // namespace brk::rdr