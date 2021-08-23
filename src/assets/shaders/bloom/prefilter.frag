#version 330 core

uniform sampler2D uSource;
uniform float uThreshold;
uniform float uKick;

in vec2 fUv;

out vec4 oColor;

void main() {
    vec3 color = texture(uSource, fUv).rgb;
    float luma = dot(vec3(0.299, 0.587, 0.114), color);
    //float luma = max(max(color.r, color.b), color.g);
    oColor = vec4(smoothstep(uThreshold - uKick, uThreshold + uKick, luma) * color,  1.0);
}