#version 330

uniform mat4 in_ProjectionMatrix;
uniform float in_StarSize;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

flat in vec4 color[1];
flat in vec2 position[1];

flat out vec4 colorGeom;
smooth out vec2 texCoordGeom;

void main() {
	for(int i = 0; i < gl_in.length(); i++) {
		for(int x = -1; x <= 1; x += 2)
			for(int y = -1; y <= 1; y += 2) {
				gl_Position = in_ProjectionMatrix * vec4(position[i].x + x * in_StarSize / 2.0, position[i].y + y * in_StarSize / 2.0, 0.0, 1.0);
				texCoordGeom = vec2( (x + 1.0) / 2.0, (y + 1.0) / 2.0 );
				colorGeom = color[i];
				EmitVertex();
			}
	}
	EndPrimitive();
}