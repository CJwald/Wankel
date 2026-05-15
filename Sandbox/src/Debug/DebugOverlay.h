#pragma once

namespace Wankel {

	class DebugOverlay {
	public:
	    static void PushFrameTime(float dt);
	    static void DrawFPSPanel();
	
	private:
	    static constexpr int s_HistorySize = 120;
	    static float s_FrameTimes[s_HistorySize];
	    static int s_FrameIndex;
	};

}
