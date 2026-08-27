#version 330 core

// Static unit quad
layout(location = 0) in vec2 aCorner; // [-0.5, 0.5]
layout(location = 1) in vec2 aUV;

// Per instance (glVertexAttribDivisor 1)
layout(location = 2) in vec3 aCenter;
layout(location = 3) in float aSize; // full billboard width, world units
layout(location = 4) in vec4 aColor;
layout(location = 5) in float aRotation; // billboard roll, radians

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;

out vec2 v_UV;
out vec4 v_Color;

void main() {
    float s = sin(aRotation);
    float c = cos(aRotation);
    vec2 corner = vec2(aCorner.x * c - aCorner.y * s, aCorner.x * s + aCorner.y * c);

    vec3 world = aCenter + (u_CameraRight * corner.x + u_CameraUp * corner.y) * aSize;

    gl_Position = u_ViewProjection * vec4(world, 1.0);
    v_UV = aUV;
    v_Color = aColor;
}
