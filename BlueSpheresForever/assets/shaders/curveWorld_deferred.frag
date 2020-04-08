#version 330

uniform sampler2D in_Shadows;
uniform sampler2D in_Texture;
uniform sampler2D in_NormalMap;

uniform vec2 in_TexCoordOffset;

uniform int in_ColorMode; // 0 - color, 1 - texture, 2 -  procedural checkerboard
uniform bool in_NormalMapping;
uniform bool in_EnableLighting;

uniform vec4 in_Color;
uniform vec4 in_SecondaryColor;

uniform float in_SpecularPower;
uniform float in_Shininess;

smooth in vec4 position;
smooth in vec2 texCoord;
smooth in vec3 normal;
smooth in vec4 positionShadow;
smooth in mat3 TBNInverse;

// #define DEBUG

#ifndef DEBUG
	out vec4 gl_FragData[gl_MaxDrawBuffers];
#else
	uniform int in_DebugMode;
	out vec4 gl_FragColor;
#endif

void main() {
	vec4 diffuseColor;
	
	vec2 texCoordPlusOffset = texCoord + in_TexCoordOffset;
		
	vec3 N;
	
	if(!in_NormalMapping)
		N = normalize(normal);
	else {		
		N = TBNInverse * normalize(texture(in_NormalMap, texCoordPlusOffset).xyz * 2.0 - 1.0);
	}
	
	switch(in_ColorMode) {
		case 0: // - color
			diffuseColor.rgb = in_Color.rgb;
			break;	
		case 1: // - texture
			diffuseColor.rgb = texture(in_Texture, texCoordPlusOffset).rgb;
			break;
		case 2: // - procedural checkerboard
			float x = fract(texCoordPlusOffset.x);
			float y = fract(texCoordPlusOffset.y);
			
			if(y < 0.5) {
				if(x < 0.5)
					diffuseColor.rgb = in_Color.rgb;
				else
					diffuseColor.rgb = in_SecondaryColor.rgb;
			} else {
				if(x < 0.5)
					diffuseColor.rgb = in_SecondaryColor.rgb;
				else
					diffuseColor.rgb = in_Color.rgb;		
			}		
			break;
	}
	
	vec3 positionShadowD = positionShadow.xyz / positionShadow.w;
	
	float zShadow = texture(in_Shadows, (positionShadowD.xy + 1.0) / 2.0).r;
	
	if(positionShadowD.z < zShadow + 0.0005)
		diffuseColor.a = 1.0; // shadow buffer
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
				gl_FragColor = vec4(diffuseColor.rgb, 1.0);
				break ;
			case 1:
				gl_FragColor = vec4(diffuseColor.a, diffuseColor.a, diffuseColor.a, 1.0);
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