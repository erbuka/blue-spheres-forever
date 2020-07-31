#version 330 core

uniform samplerCube uBackgroundPattern;
uniform vec3 uColor0;
uniform vec3 uColor1;

in vec3 fPosition;
in vec3 fUv;

out vec4 oColor;


const vec3 colors[4] = vec3[](
    vec3(1.0, 0.2, 0.3),
    vec3(0.2, 1.0, 0.3),
    vec3(0.3, 0.3, 1.0),
    vec3(0.3, 1.0, 1.0)
);

void main() {
    float factor = texture(uBackgroundPattern, fUv).r * (colors.length() - 1);
    int c0 = int(floor(factor));
    int c1 = int(ceil(factor));
    float t = factor - float(c0);
    oColor = vec4(mix(colors[c0], colors[c1], t), 1.0);
}
