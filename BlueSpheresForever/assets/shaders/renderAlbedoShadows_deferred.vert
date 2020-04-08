#version 330

uniform mat4 in_ProjectionMatrix;

layout(location = 0) in vec2 in_TexCoord;

smooth out vec2 position;
smooth out vec2 texCoord;

void main() {
	gl_Position = in_ProjectionMatrix * vec4(in_TexCoord.x * 2.0 - 1.0, in_TexCoord.y * 2.0 - 1.0, 0.0, 1.0);
	texCoord = in_TexCoord;
	position = in_TexCoord * 2.0 - 1.0;
}