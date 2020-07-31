#version 330 core

uniform sampler2D uColor;
uniform float uExposure;

in vec2 fUv;

out vec4 oColor;	


void main() {
    vec3 color = texture(uColor, fUv).rgb;
    color = vec3(1.0) - exp(-color * uExposure);
    oColor = vec4(color, 1.0);
}