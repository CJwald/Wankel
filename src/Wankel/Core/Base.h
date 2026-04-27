#pragma once

#include "Wankel/Core/PlatformDetection.h"

#include <memory>

#ifdef WK_DEBUG
	#if defined(WK_PLATFORM_WINDOWS)
		#define WK_DEBUGBREAK() __debugbreak()
	#elif defined(WK_PLATFORM_LINUX)
		#include <signal.h>
		#define WK_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define WK_ENABLE_ASSERTS
#else
	#define WK_DEBUGBREAK()
#endif

#define WK_EXPAND_MACRO(x) x
#define WK_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define WK_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Wankel {
	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
}

#include "Wankel/Core/Log.h"
#include "Wankel/Core/Assert.h"
