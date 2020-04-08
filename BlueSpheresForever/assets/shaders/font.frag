#version 330

uniform sampler2D in_Font;
uniform vec3 in_Color;

smooth in vec2 texCoord;

out vec4 gl_FragColor;

void main() {
	vec4 pix = texture(in_Font, texCoord);
	gl_FragColor = vec4(in_Color, pix.r);
}