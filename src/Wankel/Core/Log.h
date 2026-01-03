#pragma once

#include "Engine.h"
#include "spdlog/spdlog.h"


namespace Wankel {
	class Log {
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetServerLogger() { return s_ServerLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static std::shared_ptr<spdlog::logger> s_ServerLogger;
	};
}

// Core Log Macros
#define WK_CORE_TRACE(...)     ::Wankel::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define WK_CORE_INFO(...)      ::Wankel::Log::GetCoreLogger()->info(__VA_ARGS__)
#define WK_CORE_WARNING(...)   ::Wankel::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define WK_CORE_ERROR(...)     ::Wankel::Log::GetCoreLogger()->error(__VA_ARGS__)
#define WK_CORE_FATAL(...)     ::Wankel::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// Client Log Macros
#define WK_CLIENT_TRACE(...)   ::Wankel::Log::GetClientLogger()->trace(__VA_ARGS__)
#define WK_CLIENT_INFO(...)    ::Wankel::Log::GetClientLogger()->info(__VA_ARGS__)
#define WK_CLIENT_WARNING(...) ::Wankel::Log::GetClientLogger()->warn(__VA_ARGS__)
#define WK_CLIENT_ERROR(...)   ::Wankel::Log::GetClientLogger()->error(__VA_ARGS__)
#define WK_CLIENT_FATAL(...)   ::Wankel::Log::GetClientLogger()->fatal(__VA_ARGS__)

// Server Log Macros
#define WK_SERVER_TRACE(...)   ::Wankel::Log::GetServerLogger()->trace(__VA_ARGS__)
#define WK_SERVER_INFO(...)    ::Wankel::Log::GetServerLogger()->info(__VA_ARGS__)
#define WK_SERVER_WARNING(...) ::Wankel::Log::GetServerLogger()->warn(__VA_ARGS__)
#define WK_SERVER_ERROR(...)   ::Wankel::Log::GetServerLogger()->error(__VA_ARGS__)
#define WK_SERVER_FATAL(...)   ::Wankel::Log::GetServerLogger()->fatal(__VA_ARGS__)
