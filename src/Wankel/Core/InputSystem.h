#pragma once

namespace Wankel {

class InputSystem {
public:
    static bool Init();
    static void Shutdown();
    static void PollControllers();
};

} // namespace Wankel
