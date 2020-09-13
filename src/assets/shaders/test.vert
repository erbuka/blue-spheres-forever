#version 330 core

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fUv;

void main() {

    vec3 position = aPosition;
    vec3 normal = aNormal;
    vec2 uv = aUv;

    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);

    fNormal = normal;
    fPosition = position;
    fUv = uv;

}	