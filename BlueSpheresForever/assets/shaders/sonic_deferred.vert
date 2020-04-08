#version 330

uniform mat4 in_ProjectionMatrix;
uniform mat4 in_ViewMatrix;
uniform mat4 in_ModelMatrix;

uniform mat4 in_ShadowViewMatrix;
uniform mat4 in_ShadowProjectionMatrix;

uniform float in_T;

layout(location = 0) in vec3 in_Position0;
layout(location = 1) in vec3 in_Position1;
layout(location = 2) in vec3 in_Normal0;
layout(location = 3) in vec3 in_Normal1;

smooth out vec4 position;
smooth out vec3 normal;
smooth out vec4 positionShadow;

void main() {
	vec3 posObject = mix(in_Position0, in_Position1, in_T);
	vec3 norm = mix(in_Normal0, in_Normal1, in_T);
	
	vec4 posView = in_ViewMatrix * in_ModelMatrix * vec4(posObject, 1.0);
	
	gl_Position = in_ProjectionMatrix * posView;
	
	position = in_ProjectionMatrix * posView;
	positionShadow = in_ShadowProjectionMatrix * in_ShadowViewMatrix * in_ModelMatrix * vec4(posObject, 1.0);
	normal = (in_ViewMatrix * in_ModelMatrix *  vec4(norm, 0.0)).xyz;
}
