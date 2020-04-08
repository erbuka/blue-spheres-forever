#version 330

uniform sampler2D in_Texture;

uniform vec4 in_Color;

uniform bool in_UseTexture;

uniform float in_AlphaFactor;

smooth in vec2 texCoord;

out vec4 gl_FragColor;

void main() {
	if(in_UseTexture) {
		vec4 color = texture(in_Texture, texCoord);
		gl_FragColor = vec4(color.rgb, color.a * in_AlphaFactor);
	} else
		gl_FragColor = vec4(in_Color.rgb, in_Color.a * in_AlphaFactor);
}