#version 330 core

uniform samplerCube uSkyBox;

in vec3 fUv;
in vec3 fPosition;

out vec4 oColor;

void main() {
    oColor = vec4(texture(uSkyBox, fUv).rgb, 1.0);
}
