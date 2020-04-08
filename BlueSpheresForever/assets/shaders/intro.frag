#version 330

uniform vec4 in_Color;
uniform bool in_Shading;

const vec4 lightColor = vec4(1.0);
const vec3 lightPos = vec3(0.0, 0.0, 0.0);

smooth in vec3 position;
smooth in vec3 normal;
smooth in vec2 texCoord;

out vec4 gl_FragColor;

void main() {
	
	vec3 L = normalize(lightPos - position);
	vec3 N = normalize(normal);
	vec3 C = normalize(-position);
	
	float diffuseFactor = max(0.0, dot(N, L));
	float specularFactor = 0.0;
	
	if(diffuseFactor != 0.0) {
		vec3 H = normalize(L + C);
		specularFactor = max(0.0, dot(N, H));
		specularFactor = pow(specularFactor, 128.0);
	}
	
	vec4 diffuseColor = in_Color;
	
	if(in_Shading)
		gl_FragColor =  diffuseColor * lightColor * diffuseFactor + lightColor * specularFactor;
	else
		gl_FragColor = in_Color;
}