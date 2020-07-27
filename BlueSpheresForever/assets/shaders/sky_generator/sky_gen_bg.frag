#version 330 core

uniform samplerCube uBackgroundPattern;
uniform vec3 uColor0;
uniform vec3 uColor1;

in vec3 fPosition;
in vec3 fUv;

out vec4 oColor;

void main() {
    float factor = texture(uBackgroundPattern, fUv).r;
    oColor = vec4(mix(uColor0, uColor1, factor), 1.0);
}
