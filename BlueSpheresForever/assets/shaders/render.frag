#version 330

uniform vec4 in_LightAmbient;

uniform sampler2D in_Diffuse;
uniform sampler2D in_Albedo;
uniform sampler2D in_Shadows;
uniform sampler2D in_Lighting;

uniform bool in_ShadowsEnabled;

smooth in vec2 texCoord;

out vec4 gl_FragColor;

void main() {
	if(texture(in_Lighting, texCoord).r != 0.0) {
		if(in_ShadowsEnabled)
			gl_FragColor = texture(in_Diffuse, texCoord) * in_LightAmbient + texture(in_Albedo, texCoord) * texture(in_Shadows, texCoord).r;
		else
			gl_FragColor = texture(in_Diffuse, texCoord) * in_LightAmbient + texture(in_Albedo, texCoord);
	} else
		gl_FragColor = texture(in_Diffuse, texCoord);
}