#version 330 core

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

uniform mat4 uJointTransform[128];

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in uvec4 aJoints;
layout(location = 4) in vec4 aWeights;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fUv;

void main() {

    mat4 jointTransform = 
        uJointTransform[aJoints.x] * aWeights.x +
        uJointTransform[aJoints.y] * aWeights.y +
        uJointTransform[aJoints.z] * aWeights.z +
        uJointTransform[aJoints.w] * aWeights.w;

    vec3 position = (jointTransform * vec4(aPosition, 1.0)).xyz;
    vec3 normal = (jointTransform * vec4(aNormal, 0.0)).xyz;
    vec2 uv = aUv;

    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);

    fNormal = normal;
    fPosition = position;
    fUv = uv;

}	