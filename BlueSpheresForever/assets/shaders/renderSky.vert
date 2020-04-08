#version 330

uniform mat4 in_ProjectionMatrix;
uniform mat4 in_ViewMatrix;
uniform mat4 in_ModelMatrix;

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Color;
layout(location = 2) in float in_Size;

flat out vec3 colorVert;
flat out float sizeVert;

void main() {
	gl_Position = in_ProjectionMatrix * in_ViewMatrix * in_ModelMatrix * vec4(in_Position, 1.0);
	colorVert = in_Color;
	sizeVert = in_Size;
}