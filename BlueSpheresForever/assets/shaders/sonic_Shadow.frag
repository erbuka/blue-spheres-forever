#version 330

out vec4 gl_FragColor;

smooth in vec4 position;

void main() {
	gl_FragColor.r = position.z / position.w;
}