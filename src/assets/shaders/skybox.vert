#version 330 core

uniform mat4 uProjection;	
uniform mat4 uView;
uniform mat4 uModel;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aUv;

out vec3 fPosition;
out vec3 fUv;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    fUv = aUv;
    fPosition = aPosition;
}