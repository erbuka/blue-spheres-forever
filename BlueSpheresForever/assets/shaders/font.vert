#version 330

layout(location = 0) in vec2 in_Vertex;
layout(location = 1) in int in_AsciiCode;

out int asciiCode;

void main() {
	gl_Position = vec4(in_Vertex.x, in_Vertex.y, 0.0, 1.0);
	asciiCode = in_AsciiCode;	
}