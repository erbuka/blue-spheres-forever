#version 330

uniform sampler2D in_Texture;

out vec4 gl_FragColor;

flat in vec4 colorGeom;
smooth in vec2 texCoordGeom;

void main() {
	gl_FragColor = vec4(colorGeom.rgb, texture(in_Texture, texCoordGeom).r);
}