#version 330

uniform sampler2D in_Shadows;

uniform vec4 in_Color;

uniform float in_SpecularPower;
uniform float in_Shininess;
uniform bool in_EnableLighting;

smooth in vec4 position;
smooth in vec4 positionShadow;
smooth in vec3 normal;

// #define DEBUG

#ifndef DEBUG
	out vec4 gl_FragData[gl_MaxDrawBuffers];
#else
	uniform int in_DebugMode;
	out vec4 gl_FragColor;
#endif

void main() {

	vec4 diffuseColor;
	
	vec3 N = normalize(normal); // normal
	
	vec3 positionShadowD = positionShadow.xyz / positionShadow.w;
	
	float zShadow = texture(in_Shadows, (positionShadowD.xy + 1.0) / 2.0).r;
	
	diffuseColor.rgb = in_Color.rgb;

	if(positionShadowD.z < zShadow + 0.0005)
		diffuseColor.a = 1.0;
	else
		diffuseColor.a = 0.0;		

	#ifndef DEBUG
		
		gl_FragData[0] = diffuseColor; // diffuse buffer
		gl_FragData[1].rgb = (N + 1.0) / 2.0; // normals buffer
		gl_FragData[1].a = in_SpecularPower; // specular power
		gl_FragData[2].r = position.z / position.w; // z-buffer
		gl_FragData[3].r = in_EnableLighting ? 1.0 : 0.0;
	#else
		
		switch(in_DebugMode) {
			case 0:
				gl_FragColor = in_Color;
				break ;
			case 1:
				gl_FragColor = vec4(diffuseColor.a);
				break ;
			case 2:
				gl_FragColor = vec4((N + 1.0) / 2.0, 1.0);
				break ;
			case 3:
				gl_FragColor = vec4(position.z / position.w);
				break ;
		}
		
	#endif
}