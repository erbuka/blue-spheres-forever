#version 330

uniform sampler2D in_Texture;

smooth in vec2 texCoord;

out vec4 gl_FragColor;

void main() {
	gl_FragColor = texture(in_Texture, texCoord);
}