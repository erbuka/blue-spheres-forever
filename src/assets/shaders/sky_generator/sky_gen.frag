#version 330 core

uniform vec3 uBackColor;
uniform vec3 uStarColor;

uniform float uBackgroundScale;

uniform float uStarBrightnessScale;
uniform float uStarScale;
uniform int uStarPower;
uniform float uStarMultipler;

uniform float uCloudScale;

uniform float uExposure;

in vec3 fPosition;
in vec3 fUv;

out vec4 oColor;

const vec3 MOD3 = vec3(.1031,.11369,.13787);
const int Octaves = 5;

float simplexNoise(vec3 p);
float clouds(vec3 p);
float absClouds(vec3 p);
float stupidPow(float x, int y);

void main() {
    vec3 dir = normalize(fPosition);

    float skyVal = simplexNoise(dir * uBackgroundScale) * 0.5 + 0.5;
    float skyVal2 = simplexNoise(dir * uBackgroundScale / 2.0) * 0.5 + 0.5;

    // Base background
    vec3 background0 = uBackColor;
    vec3 background1 = mix(vec3(1.0, 1.0, 0.2), vec3(1.0, 0.2, 1.0), skyVal2);
    vec3 background = mix(background1, background0, skyVal);

    // Stars
    float starBrightness = simplexNoise(dir * uStarBrightnessScale) * 0.5 + 0.5;
    float starVal = simplexNoise(dir * uStarScale) * 0.5 + 0.5;
    //starVal = pow(starVal * starBrightness, uStarPower) * uStarMultipler;
    starVal = stupidPow(starVal * starBrightness, uStarPower) * uStarMultipler;
    vec3 starColor = uStarColor * starVal;

    // Clouds
    float cloudVal = clouds(dir * uCloudScale) * 0.5 + 0.5;
    float cloudStep = smoothstep(0.0, 0.1, clouds(dir * uCloudScale));

    float lighting = (background.r + background.g + background.b) / 3.0;
    
    vec3 clouds = mix(background, vec3(1.0f), cloudVal) * lighting;

    vec3 color = background + starColor * (1.0 - cloudStep) + clouds * cloudStep;

    color = vec3(1.0) - exp(-color * uExposure);

    oColor = vec4(color, 1.0);

}

float stupidPow(float x, int y)
{
    float result = 1.0;
    while(y > 0)
    {
        result *= x;
        --y;
    }
    return result;

}

float hash31(vec3 p3) {
	p3  = fract(p3 * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return -1.0 + 2.0 * fract((p3.x + p3.y) * p3.z);
}

vec3 hash33(vec3 p3) {
	p3 = fract(p3 * MOD3);
    p3 += dot(p3, p3.yxz+19.19);
    return -1.0 + 2.0 * fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

float clouds(vec3 p) {
    float m = 0.0f;
    float v = 0.0f;
    float f = 1.0f;

    for(int i = 0; i < Octaves; ++i) {
        v += simplexNoise(p) * f;
        m += f;
        f *= 0.5;
        p *= 2.0;
    }

    return v / m;
}

float absClouds(vec3 p) {
    float m = 0.0f;
    float v = 0.0f;
    float f = 1.0f;

    for(int i = 0; i < Octaves; ++i) {
        v += abs(simplexNoise(p) * f);
        m += f;
        f *= 0.5;
        p *= 2.0;
    }

    return v / m;
}


float simplexNoise(vec3 p) {
    const float K1 = 0.333333333;
    const float K2 = 0.166666667;
    
    vec3 i = floor(p + (p.x + p.y + p.z) * K1);
    vec3 d0 = p - (i - (i.x + i.y + i.z) * K2);
    
    // thx nikita: https://www.shadertoy.com/view/XsX3zB
    vec3 e = step(vec3(0.0), d0 - d0.yzx);
	vec3 i1 = e * (1.0 - e.zxy);
	vec3 i2 = 1.0 - e.zxy * (1.0 - e);
    
    vec3 d1 = d0 - (i1 - 1.0 * K2);
    vec3 d2 = d0 - (i2 - 2.0 * K2);
    vec3 d3 = d0 - (1.0 - 3.0 * K2);
    
    vec4 h = max(0.6 - vec4(dot(d0, d0), dot(d1, d1), dot(d2, d2), dot(d3, d3)), 0.0);
    vec4 n = h * h * h * h * vec4(dot(d0, hash33(i)), dot(d1, hash33(i + i1)), dot(d2, hash33(i + i2)), dot(d3, hash33(i + 1.0)));
    
    return dot(vec4(31.316), n);
}
