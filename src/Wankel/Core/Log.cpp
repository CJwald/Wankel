#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Wankel {

std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
std::shared_ptr<spdlog::logger> Log::s_ServerLogger;
std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> Log::s_ConsoleSink;

void Log::Init() {
    spdlog::set_pattern("%^[%T] %n: %v%$");
    s_CoreLogger = spdlog::stdout_color_st("WANKEL");
    s_CoreLogger->set_level(spdlog::level::trace);

    s_ClientLogger = spdlog::stdout_color_st("WANKEL CLIENT");
    s_ClientLogger->set_level(spdlog::level::trace);

    s_ServerLogger = spdlog::stdout_color_st("WANKEL SERVER");
    s_ServerLogger->set_level(spdlog::level::trace);

    // Console panel feed (see GetConsoleSink) - Core/Client only, matching those two loggers'
    // scope; a client game has no Server logger traffic worth surfacing here.
    s_ConsoleSink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(1000);
    s_ConsoleSink->set_level(spdlog::level::trace);
    s_CoreLogger->sinks().push_back(s_ConsoleSink);
    s_ClientLogger->sinks().push_back(s_ConsoleSink);
};
} // namespace Wankel
