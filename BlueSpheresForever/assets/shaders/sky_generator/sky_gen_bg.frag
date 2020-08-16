#version 330 core

uniform samplerCube uBackgroundPattern;
uniform sampler2D uGradient;

in vec3 fPosition;
in vec3 fUv;

out vec4 oColor;

void main() {
    float t = texture(uBackgroundPattern, fUv).r;
    oColor = vec4(texture(uGradient, vec2(t, 0.0)).rgb, 1.0);
}
