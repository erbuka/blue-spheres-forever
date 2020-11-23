#version 330

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

uniform vec2 uUvOffset;

#if defined(SKELETAL)

uniform mat4 uJointTransform[128];

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in uvec4 aJoints;
layout(location = 4) in vec4 aWeights;

#else

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;

#endif

out vec3 fPosition;
out vec3 fNormal;
out vec2 fUv;

void main() {

    #if defined(SKELETAL)
    
    mat4 jointTransform = 
        uJointTransform[aJoints.x] * aWeights.x +
        uJointTransform[aJoints.y] * aWeights.y +
        uJointTransform[aJoints.z] * aWeights.z +
        uJointTransform[aJoints.w] * aWeights.w;

    vec3 position = (jointTransform * vec4(aPosition, 1.0)).xyz;
    vec3 normal = (jointTransform * vec4(aNormal, 0.0)).xyz;
    vec2 uv = aUv;

    #else

    vec3 position = aPosition;
    vec3 normal = aNormal;
    vec2 uv = aUv;

    #endif

    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);

    fNormal = normal;
    fPosition = position;

    #ifdef NO_UV_OFFSET 
    fUv = uv;
    #else
    fUv = uv + uUvOffset;
    #endif

}	