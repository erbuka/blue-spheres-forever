#version 330

uniform vec2 in_PixelStep;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

flat in vec3 colorVert[1];
flat in float sizeVert[1];

smooth out vec2 texCoordGeom;
flat out vec3 colorGeom;

void main() {

	vec4 startPos = gl_in[0].gl_Position / gl_in[0].gl_Position.w;
	
	for(int x = -1; x <= 1; x += 2)
		for(int y = -1; y <= 1; y += 2) {
			gl_Position =  startPos + vec4(x * sizeVert[0] * in_PixelStep.x, y * sizeVert[0] * in_PixelStep.y, 0.0, 0.0);
			texCoordGeom = vec2( (x + 1.0) / 2.0, (y + 1.0) / 2.0 );
			colorGeom = colorVert[0];
			EmitVertex();
		}
	EndPrimitive();
}