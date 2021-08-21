#version 330

uniform sampler2D uSource;
uniform float uThreshold;
uniform float uKick;

in vec2 fUv;

out vec4 oColor;

void main() {
    vec3 color = texture(uSource, fUv).rgb;
    float luma = dot(vec3(0.299, 0.587, 0.114), color);
    oColor = vec4(smoothstep(uThreshold - uKick, uThreshold + uKick, luma) * color,  1.0);
}