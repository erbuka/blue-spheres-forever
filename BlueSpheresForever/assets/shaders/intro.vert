#version 330

uniform mat4 in_ProjectionMatrix;
uniform mat4 in_ModelViewMatrix;

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;

smooth out vec3 position;
smooth out vec3 normal;
smooth out vec2 texCoord;

void main() {
	vec4 posView = in_ModelViewMatrix * vec4(in_Position, 1.0);
	gl_Position = in_ProjectionMatrix * posView;
	
	position = posView.xyz;
	normal = (in_ModelViewMatrix * vec4(in_Normal, 0.0)).xyz;
	texCoord = in_TexCoord;
}