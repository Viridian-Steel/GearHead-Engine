#pragma once

#include "Core.h"

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)


namespace GearHead{
    class GEARHEAD_API Log {
        public:
            static void Init();

            inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		    inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

        private:
            static std::shared_ptr<spdlog::logger> s_CoreLogger;
            static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

//Core log macros
#define GEARHEAD_CORE_ERROR(...)     ::GearHead::Log::GetCoreLogger()->error(__VA_ARGS__)
#define GEARHEAD_CORE_WARN(...)      ::GearHead::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define GEARHEAD_CORE_INFO(...)      ::GearHead::Log::GetCoreLogger()->info(__VA_ARGS__)
#define GEARHEAD_CORE_CRITICAL(...)  ::GearHead::Log::GetCoreLogger()->critical(__VA_ARGS__)
#define GEARHEAD_CORE_TRACE(...)     ::GearHead::Log::GetCoreLogger()->trace(__VA_ARGS__)


//Client log macros
#define GEARHEAD_ERROR(...)          ::GearHead::Log::GetClientLogger()->error(__VA_ARGS__)
#define GEARHEAD_WARN(...)           ::GearHead::Log::GetClientLogger()->warn(__VA_ARGS__)
#define GEARHEAD_INFO(...)	         ::GearHead::Log::GetClientLogger()->info(__VA_ARGS__)
#define GEARHEAD_CRITICAL(...)       ::GearHead::Log::GetClientLogger()->critical(__VA_ARGS__)
#define GEARHEAD_TRACE(...)          ::GearHead::Log::GetClientLogger()->trace(__VA_ARGS__)
