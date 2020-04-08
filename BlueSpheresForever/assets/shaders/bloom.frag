#version 330

uniform sampler2D in_Src;
uniform sampler2D in_Depth;

uniform float in_Step;
uniform float in_Exposure;
uniform bool in_Horizontal;

const float kernel[9] = float[] 
	(0.05,  0.09, 0.12, 0.15, 0.16, 0.15, 0.12, 0.09, 0.05);

smooth in vec2 texCoord;

void main() {
	vec4 accum = vec4(0.0);
	vec4 color;
	
	float depth;
	
	if(in_Horizontal) {
		for(int k = 0; k < 9; k++)
			accum += kernel[k] * texture(in_Src, texCoord + vec2(in_Step * (k - 4), 0.0));		
	} else {
		for(int k = 0; k < 9; k++)
			accum += kernel[k] * texture(in_Src, texCoord + vec2(0.0, in_Step * (k - 4)));	
	}
	
	color = texture(in_Src, texCoord);
	depth = (texture(in_Depth, texCoord).r + 1.0) / 2.0;
	
	if(depth < 0.98) {
		depth = 0.0;
	} else {
		depth = (depth - 0.98) / 0.02;
	}

	gl_FragColor = mix(color, accum, depth) + accum * accum * in_Exposure;	

}