#version 330 core

uniform sampler2D uSource;

in vec2 fUv;

out vec4 oColor;

void main() {
    vec2 s = 1.0 / vec2(textureSize(uSource, 0));

    vec3 tl = texture(uSource, fUv + vec2(-s.x, +s.y)).rgb;
    vec3 tr = texture(uSource, fUv + vec2(+s.x, +s.y)).rgb;
    vec3 bl = texture(uSource, fUv + vec2(-s.x, -s.y)).rgb;
    vec3 br = texture(uSource, fUv + vec2(+s.x, -s.y)).rgb;

    oColor = vec4((tl + tr + bl + br) / 4.0,  1.0);
    //oColor = vec4(1.0, 0.0, 0.0, 1.0);
}