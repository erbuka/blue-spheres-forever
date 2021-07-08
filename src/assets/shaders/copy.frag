#version 330

uniform sampler2D uSource;
in vec2 fUv;

out vec4 oColor;

void main() {
    oColor = texture(uSource, fUv);
}