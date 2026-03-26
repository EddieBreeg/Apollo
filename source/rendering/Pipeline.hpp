#pragma once

/** \file Pipeline.hpp */

#include <PCH.hpp>

#include "VertexTypes.hpp"
#include <core/JsonFwd.hpp>

struct SDL_GPUGraphicsPipelineCreateInfo;

/** JSON converter for SDL_GPUGraphicsPipelineCreateInfo, used internally to load materials from
 * disk */
template <>
struct apollo::json::Converter<SDL_GPUGraphicsPipelineCreateInfo>
{
	APOLLO_API static bool FromJson(
		SDL_GPUGraphicsPipelineCreateInfo& out_info,
		const nlohmann::json& json) noexcept;
	APOLLO_API static void ToJson(
		const SDL_GPUGraphicsPipelineCreateInfo& info,
		nlohmann::json& out_json) noexcept;
};

namespace apollo::rdr {
	/** \brief Maximum number of color targets for a graphics pipeline output */
	static constexpr uint32 MaxColorTargets = 8;
} // namespace apollo::rdr