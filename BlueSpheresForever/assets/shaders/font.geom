#version 330

const float stepTC = 1.0/16.0;

uniform mat4 in_ModelMatrix;
uniform mat4 in_ProjectionMatrix;

uniform float in_Size;

layout(points) in;
layout(triangle_strip, max_vertices = 16) out;

in int asciiCode[1];

smooth out vec2 texCoord;

void main() {
	
	for(int i = 0; i < 2; i++)
		for(int j = 0; j < 2; j++) {	
	
			texCoord = vec2(asciiCode[0] * stepTC + i * stepTC,
					asciiCode[0] / 16 * stepTC + (j == 1 ? 0 : 1) * stepTC);

			gl_Position = in_ProjectionMatrix * in_ModelMatrix *
								vec4(gl_in[0].gl_Position.x + i * in_Size,
								gl_in[0].gl_Position.y + j * in_Size,
								0.0, 1.0);
			EmitVertex();
			
		}
	EndPrimitive();
}