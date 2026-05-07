#pragma once

namespace Wankel {

    enum class GamepadAxis
    {
        LeftX = 0,
        LeftY,
        RightX,
        RightY,
        L2,
        R2
    };

    enum class GamepadButton {
        Cross = 0,     // A on Xbox
        Circle,        // B
        Square,        // X
        Triangle,      // Y

        DPadUp,
        DPadDown,
        DPadLeft,
        DPadRight,

        L1,
        R1,

        L3,
        R3,

        Start,
        Options,

        PS
    };

}
