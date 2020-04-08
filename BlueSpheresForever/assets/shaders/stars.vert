#version 330

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec4 in_Color;

flat out vec2 position;
flat out vec4 color;

void main() {
	position = in_Position;
	color = in_Color;
}