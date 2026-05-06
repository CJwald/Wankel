#pragma once

namespace Wankel {

    class InputSystem {
    public:
        static void Init();
        static void Shutdown();
        static void PollControllers();
    };

}
