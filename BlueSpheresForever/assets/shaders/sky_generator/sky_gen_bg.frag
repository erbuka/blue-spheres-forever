#version 330 core

uniform vec3 uColor;

in vec3 fPosition;
in vec3 fUv;

out vec4 oColor;

void main() {
    oColor = vec4(uColor, 1.0);
}
