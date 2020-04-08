#version 330

uniform vec4 in_Color0;
uniform vec4 in_Color1;

smooth in vec2 texCoord;

out vec4 gl_FragData[gl_MaxDrawBuffers];

void main() {
	gl_FragData[0] = mix(in_Color1, in_Color0, clamp((texCoord.y - 0.5) * 2, 0, 1));
	gl_FragData[3].r = 0.0; // Disable lighting;
}