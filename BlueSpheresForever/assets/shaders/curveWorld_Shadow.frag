#version 330

smooth in vec4 position;

out vec4 gl_FragColor;

void main() {
	gl_FragColor.r = position.z / position.w;
}