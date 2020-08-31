#version 330

uniform sampler2D uSource;
uniform bool uHorizontal;

in vec2 fUv;

out vec4 oColor;

//const float[] kernel = float[](0.06136, 0.24477, 0.38774, 0.24477, 0.06136);
const float[] kernel = float[](0.066414, 0.079465, 0.091364, 0.100939, 0.107159, 0.109317, 0.107159, 0.100939, 0.091364, 0.079465, 0.066414);


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