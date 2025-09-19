#pragma once

#include <spdlog/spdlog.h>

#ifdef BRK_DEV
#define BRK_LOG(level, ...)                                                                        \
	spdlog::log(spdlog::source_loc{ __FILE__, __LINE__, __func__ }, level, __VA_ARGS__)
#define BRK_LOG_TRACE(...)	  BRK_LOG(spdlog::level::trace, __VA_ARGS__)
#define BRK_LOG_INFO(...)	  BRK_LOG(spdlog::level::info, __VA_ARGS__)
#define BRK_LOG_WARN(...)	  BRK_LOG(spdlog::level::warn, __VA_ARGS__)
#define BRK_LOG_ERROR(...)	  BRK_LOG(spdlog::level::err, __VA_ARGS__)
#define BRK_LOG_CRITICAL(...) BRK_LOG(spdlog::level::critical, __VA_ARGS__)
#else
#define BRK_LOG(level, ...)	  (void)level
#define BRK_LOG_TRACE(...)	  (void)0
#define BRK_LOG_INFO(...)	  (void)0
#define BRK_LOG_WARN(...)	  (void)0
#define BRK_LOG_ERROR(...)	  (void)0
#define BRK_LOG_CRITICAL(...) (void)0
#endif