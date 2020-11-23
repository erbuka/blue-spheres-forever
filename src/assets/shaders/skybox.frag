#version 330

uniform samplerCube uSkyBox;

in vec3 fUv;
in vec3 fPosition;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oEmission;

void main() {
    oColor = vec4(texture(uSkyBox, fUv).rgb, 1.0);
    oEmission = vec4(0.0, 0.0, 0.0, 1.0);
}
