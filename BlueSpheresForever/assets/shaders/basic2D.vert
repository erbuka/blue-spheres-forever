#version 330

uniform mat4 in_ProjectionMatrix;
uniform mat4 in_ModelMatrix;

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;

smooth out vec2 texCoord;

void main() {
	gl_Position = in_ProjectionMatrix * in_ModelMatrix * vec4(in_Position.x, in_Position.y, 0.0, 1.0);
	texCoord = in_TexCoord;
}