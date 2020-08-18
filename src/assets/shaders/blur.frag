#version 330

uniform sampler2D uSource;
uniform bool uHorizontal;

in vec2 fUv;

out vec4 oColor;

//const float[] kernel = float[](0.06136, 0.24477, 0.38774, 0.24477, 0.06136);
const float[] kernel = float[](0.0093, 0.028002, 0.065984, 0.121703, 0.175713, 0.198596, 0.175713, 0.121703, 0.065984, 0.028002, 0.0093);


void main() {

    vec2 texelSize = 1.0 / textureSize(uSource, 0);
    vec3 color = vec3(0.0);
    float offset = float(kernel.length / 2);

    if(uHorizontal) {
        for(int i = 0; i < kernel.length; i++)
            color += texture(uSource, fUv + vec2(texelSize.x, 0.0) * (float(i) - offset)).rgb * kernel[i];
    } else {
        for(int i = 0; i < kernel.length; i++)
            color += texture(uSource, fUv + vec2(0.0, texelSize.y) * (float(i) - offset)).rgb * kernel[i];
    }

    oColor = vec4(color, 1.0);
}