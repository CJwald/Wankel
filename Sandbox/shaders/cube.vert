#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

out vec4 v_Color;
out vec3 v_WorldPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	vec4 worldPos = model * vec4(aPos, 1.0);
    v_WorldPos = worldPos.xyz; 

	v_Color = aColor;
    gl_Position = projection * view * worldPos;
}
