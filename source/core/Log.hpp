#pragma once

#include <spdlog/spdlog.h>

#ifdef APOLLO_DEV
#define APOLLO_LOG(level, ...)                                                                     \
	spdlog::log(spdlog::source_loc{ __FILE__, __LINE__, __func__ }, level, __VA_ARGS__)
#define APOLLO_LOG_TRACE(...)	 APOLLO_LOG(spdlog::level::trace, __VA_ARGS__)
#define APOLLO_LOG_INFO(...)	 APOLLO_LOG(spdlog::level::info, __VA_ARGS__)
#define APOLLO_LOG_WARN(...)	 APOLLO_LOG(spdlog::level::warn, __VA_ARGS__)
#define APOLLO_LOG_ERROR(...)	 APOLLO_LOG(spdlog::level::err, __VA_ARGS__)
#define APOLLO_LOG_CRITICAL(...) APOLLO_LOG(spdlog::level::critical, __VA_ARGS__)
#else
#define APOLLO_LOG(level, ...)	 (void)level
#define APOLLO_LOG_TRACE(...)	 (void)0
#define APOLLO_LOG_INFO(...)	 (void)0
#define APOLLO_LOG_WARN(...)	 (void)0
#define APOLLO_LOG_ERROR(...)	 (void)0
#define APOLLO_LOG_CRITICAL(...) (void)0
#endif

#include "ULIDFormatter.hpp"