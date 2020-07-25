#version 330 core
	
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

uniform vec2 uUvOffset;

uniform float uMorphDelta;

layout(location = 0) in vec3 aPosition0;
layout(location = 1) in vec3 aNormal0;
layout(location = 2) in vec2 aUv0;

layout(location = 3) in vec3 aPosition1;
layout(location = 4) in vec3 aNormal1;
layout(location = 5) in vec2 aUv1;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fUv;

void main() {

    vec3 position = mix(aPosition0, aPosition1, uMorphDelta);
    vec3 normal = normalize(mix(aNormal0, aNormal1, uMorphDelta));
    vec2 uv = mix(aUv0, aUv1, uMorphDelta);

    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);

    fNormal = normal;
    fPosition = position;

    #ifdef NO_UV_OFFSET 
    fUv = uv;
    #else
    fUv = uv + uUvOffset;
    #endif

}	