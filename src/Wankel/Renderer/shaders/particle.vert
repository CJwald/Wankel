#version 330 core

// Static unit quad - corner.x is the long axis for the velocity-aligned modes
layout(location = 0) in vec2 aCorner; // [-0.5, 0.5]
layout(location = 1) in vec2 aUV;

// Per instance (glVertexAttribDivisor 1)
layout(location = 2) in vec3 aCenter;
layout(location = 3) in float aSize; // quad width, world units
layout(location = 4) in vec3 aVelocity;
layout(location = 5) in float aRotation; // billboard roll, radians
layout(location = 6) in vec4 aColor;
layout(location = 7) in vec4 aUVRect; // u0, v0, du, dv

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;
uniform vec3 u_CameraPos;

// Per batch (one layer) - see Wankel::ParticleOrientation
uniform int u_Orientation; // 0 Billboard, 1 VelocityAligned, 2 VelocityStretched
uniform float u_Aspect;
uniform float u_StretchFactor;

out vec2 v_UV;
out vec4 v_Color;

void main() {
    vec3 world;
    if (u_Orientation == 0) {
        float s = sin(aRotation);
        float c = cos(aRotation);
        vec2 corner = vec2(aCorner.x * c - aCorner.y * s, aCorner.x * s + aCorner.y * c);
        world = aCenter + (u_CameraRight * corner.x + u_CameraUp * corner.y) * aSize;
    } else {
        // Long axis along the velocity, short axis perpendicular to it and to the view ray, so the quad
        // stays as face-on as possible while pointing where the particle is going.
        float speed = length(aVelocity);
        vec3 axis = speed > 1e-5 ? aVelocity / speed : u_CameraUp;
        vec3 side = cross(normalize(u_CameraPos - aCenter), axis); // keeps the quad front-facing
        float sideLength = length(side);
        side = sideLength > 1e-5 ? side / sideLength : u_CameraRight;

        float len = aSize * u_Aspect + (u_Orientation == 2 ? speed * u_StretchFactor : 0.0);
        world = aCenter + axis * (aCorner.x * len) + side * (aCorner.y * aSize);
    }

    gl_Position = u_ViewProjection * vec4(world, 1.0);
    v_UV = aUVRect.xy + aUV * aUVRect.zw;
    v_Color = aColor;
}
