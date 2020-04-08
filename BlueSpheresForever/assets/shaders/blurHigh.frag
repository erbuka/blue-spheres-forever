#version 330

uniform sampler2D in_RTexture;

uniform float in_Step;
uniform bool in_Horizontal;

const float kernel[9] = float[] 
	(0.05,  0.09, 0.12, 0.15, 0.16, 0.15, 0.12, 0.09, 0.05);

smooth in vec2 texCoord;

out vec4 gl_FragColor;

void main() {
	float accum = 0.0;
	if(in_Horizontal) {
		for(int k = 0; k < 9; k++)
			accum += kernel[k] * texture(in_RTexture, texCoord + vec2(in_Step * (k - 4), 0.0)).r;
	} else {
		for(int k = 0; k < 9; k++)
			accum += kernel[k] * texture(in_RTexture, texCoord + vec2(0.0, in_Step * (k - 4))).r;		
	}
	
	gl_FragColor.r = accum;
}