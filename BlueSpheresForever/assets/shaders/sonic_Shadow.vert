#version 330

uniform mat4 in_ProjectionMatrix;
uniform mat4 in_ViewMatrix;
uniform mat4 in_ModelMatrix;

uniform float in_T;

layout(location = 0) in vec3 in_Position0;
layout(location = 1) in vec3 in_Position1;

smooth out vec4 position;

void main() {
	vec4 pos = in_ProjectionMatrix * in_ViewMatrix * in_ModelMatrix * 
		vec4(mix(in_Position0, in_Position1, in_T), 1.0);
	
	gl_Position = pos;
	
	position = pos;
}
