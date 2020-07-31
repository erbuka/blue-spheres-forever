#version 330

uniform samplerCube uStarsPattern;

in vec3 fNormal;
in vec3 fUv;

out vec4 oColor;

void main() {
    vec3 color = texture(uStarsPattern, fUv).rgb;
    float avg = (color.r + color.b + color.g) / 3.0;
    oColor = vec4(vec3(1.0), avg);
}