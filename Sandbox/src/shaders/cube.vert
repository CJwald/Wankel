#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec3 aNormal;

out vec4 v_Color;
out vec3 v_WorldPos;
out vec3 v_Normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 u_NormalMatrix;

void main() {
	vec4 worldPos = model * vec4(aPos, 1.0);
    v_WorldPos = worldPos.xyz;

	v_Color = aColor;
    v_Normal = normalize(u_NormalMatrix * aNormal);
    gl_Position = projection * view * worldPos;
}
