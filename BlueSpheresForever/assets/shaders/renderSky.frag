#version 330

uniform sampler2D in_Texture;

smooth in vec2 texCoordGeom;
flat in vec3 colorGeom;

out vec4 gl_FragData[gl_MaxDrawBuffers];

const vec3 WHITE = vec3(1.0, 1.0, 1.0);

void main() {
	float factor = texture(in_Texture, texCoordGeom).r;
	gl_FragData[0] = vec4(mix(colorGeom, WHITE, factor), factor); // sky color (alpha blending on)
	gl_FragData[3].r = 0.0; // disable lighting
	// All the other draw buffers doesn't matter
}