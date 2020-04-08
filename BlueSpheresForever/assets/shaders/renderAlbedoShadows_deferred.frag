#version 330

uniform mat4 in_ProjectionMatrixInverse;

uniform sampler2D in_DiffuseAndShadows;
uniform sampler2D in_Normals;
uniform sampler2D in_Depth;

uniform bool in_ShadowsEnabled;

uniform vec4 in_LightPosition;
uniform vec4 in_LightAmbient;
uniform vec4 in_LightDiffuse;
uniform vec4 in_LightSpecular;
uniform vec3 in_LightAttenuation;

uniform vec2 in_BlurStep;

smooth in vec2 position;
smooth in vec2 texCoord;

out vec4 gl_FragData[gl_MaxDrawBuffers];

void main() {
	vec4 diffuseAndShadows = texture(in_DiffuseAndShadows, texCoord);
	vec4 normalAndSpecPow = texture(in_Normals, texCoord);
	vec3 N = normalize(normalAndSpecPow.xyz * 2.0 - 1.0);
	float depth = texture(in_Depth, texCoord).r;
	
	vec4 posView = in_ProjectionMatrixInverse * vec4(position.x, position.y, depth, 1.0);
	
	posView /= posView.w;
	
	vec3 L = in_LightPosition.xyz - posView.xyz;
	
	// temp
	float dist = length(L);
	
	float attenuation;
	
	if(dist < in_LightAttenuation.z)
		attenuation = 1.0 / ( 1.0 + dist * in_LightAttenuation.x );
	else
		attenuation = 1.0 / ( 1.0 + in_LightAttenuation.z * in_LightAttenuation.x + (dist - in_LightAttenuation.z) * in_LightAttenuation.y );
	
	// temp
	
	L = normalize(L);
	
	vec3 C = normalize(-posView.xyz);
	
	float diffuseFactor = max(0.0, dot(L, N)) * attenuation;
	float specularFactor = 0.0;
	
	if(diffuseFactor != 0.0) {
		vec3 H = normalize(C + L);
		specularFactor = max(0.0, dot(N, H));
		specularFactor = pow(specularFactor, 128.0) * normalAndSpecPow.a * attenuation;
	}
	
	gl_FragData[0].rgb = (vec4(diffuseAndShadows.rgb, 1.0) * in_LightDiffuse * diffuseFactor + in_LightSpecular * specularFactor).rgb; 
	
	if(in_ShadowsEnabled) {
		float shadow = 0.0;
		
		for(int i = 0; i < 3; i++)
			for(int j = 0; j < 3; j++)
				shadow += 1.0 / 9.0 * texture(in_DiffuseAndShadows, texCoord + vec2(in_BlurStep.x * (i - 1), in_BlurStep.y * (j - 1))).a;
	
		gl_FragData[1].r = shadow;
	}
	
}