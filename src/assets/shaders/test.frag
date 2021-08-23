#version 330 core

uniform sampler2D uMap;

uniform vec4 uColor;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

in vec3 fPosition;
in vec3 fNormal;
in vec2 fUv;



out vec4 oColor;

void main()
{
    vec3 normal = normalize(mat3(uView * uModel) * fNormal);

    float factor = max(0.0, dot(normal, vec3(0.0, 0.0, 1.0)));

    oColor = texture(uMap, fUv) * uColor * factor;
}