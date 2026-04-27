#pragma once

// WANKEL PRECOMPILED HEADER

#include "Wankel/Core/PlatformDetection.h"

#ifdef WK_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
#endif

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Wankel/Core/Base.h"
#include "Wankel/Core/Log.h"



