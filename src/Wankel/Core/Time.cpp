#include "Wankel/Core/Time.h"
#include <GLFW/glfw3.h>

double Time::GetTime() {
    return glfwGetTime();
}
