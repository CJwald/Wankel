#version 330 core

out vec4 FragColor;

// Occlusion-query proxy only - drawn with color writes disabled (SetColorWrite(false)) and depth
// writes off; the sample count is all that matters.
void main() {
    FragColor = vec4(1.0);
}
