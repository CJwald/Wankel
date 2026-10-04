#pragma once

class Time {
public:
    // Seconds since startup. Double on purpose: a float loses sub-millisecond precision after a couple of
    // hours, which makes frame dt (and everything integrated with it) visibly jittery in long sessions.
    static double GetTime();
};
