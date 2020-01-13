#version 450 core

layout (location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;

void main()
{
	gl_Position = u_ViewProjection * vec4(a_Position, 1.0f);
}