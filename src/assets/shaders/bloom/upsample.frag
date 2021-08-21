#version 330

uniform sampler2D uPrevious;
uniform sampler2D uPrefilter;

in vec2 fUv;

out vec4 oColor;

void main() {
    vec2 s = 1.0 / vec2(textureSize(uPrefilter, 0));

    vec3 upsampleColor = vec3(0.0);

    upsampleColor += 1.0 * texture(uPrefilter, fUv + vec2(-s.x, +s.y)).rgb;
    upsampleColor += 2.0 * texture(uPrefilter, fUv + vec2(+0.0, +s.y)).rgb;
    upsampleColor += 1.0 * texture(uPrefilter, fUv + vec2(+s.x, +s.y)).rgb;
    upsampleColor += 2.0 * texture(uPrefilter, fUv + vec2(-s.x, +0.0)).rgb;
    upsampleColor += 4.0 * texture(uPrefilter, fUv + vec2(+0.0, +0.0)).rgb;
    upsampleColor += 2.0 * texture(uPrefilter, fUv + vec2(+s.x, +0.0)).rgb;
    upsampleColor += 1.0 * texture(uPrefilter, fUv + vec2(-s.x, -s.y)).rgb;
    upsampleColor += 2.0 * texture(uPrefilter, fUv + vec2(+0.0, -s.y)).rgb;
    upsampleColor += 1.0 * texture(uPrefilter, fUv + vec2(+s.x, -s.y)).rgb;
    
    oColor = vec4(upsampleColor / 16.0 + texture(uPrevious, fUv).rgb, 1.0);

}