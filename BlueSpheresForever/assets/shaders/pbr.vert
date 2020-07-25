#version 330 core

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

uniform vec2 uUvOffset;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fUv;

void main() {
    
    vec3 position = (uView * uModel * vec4(aPosition, 1.0)).xyz;
    vec2 uv = aUv;

    gl_Position = uProjection * vec4(position, 1.0);

    fNormal = aNormal;
    fPosition = aPosition;

    #ifdef NO_UV_OFFSET 
    fUv = uv;
    #else
    fUv = uv + uUvOffset;
    #endif

}	