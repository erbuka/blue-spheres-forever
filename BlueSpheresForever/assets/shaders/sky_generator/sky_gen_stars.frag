#version 330

uniform sampler2D uTexture;
uniform vec3 uColor;

in vec2 fUv;
out vec4 oColor;

void main() {
    oColor = texture(uTexture, fUv) * vec4(uColor, 1.0);
}