#version 330

uniform sampler2D in_RTexture;

uniform float in_Step;
uniform bool in_Horizontal;

const float kernel[3] = float[]
	(0.125, 0.75, 0.125);

smooth in vec2 texCoord;

out vec4 gl_FragColor;

void main() {
	float accum = 0.0;
	
	if(in_Horizontal) {
		for(int k = 0; k < 3; k++)
			accum += kernel[k] * texture(in_RTexture, texCoord + vec2(in_Step * (k - 1), 0.0)).r;
	} else {
		for(int k = 0; k < 3; k++)
			accum += kernel[k] * texture(in_RTexture, texCoord + vec2(0.0, in_Step * (k - 1))).r;		
	}
	
	gl_FragColor.r = accum;
}