#version 330

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

uniform vec3 uCameraPos;
uniform vec3 uLightPos;

uniform vec4 uColor;

uniform float uLightRadiance;

uniform float uEmission;

uniform sampler2D uMap;
uniform sampler2D uMetallic;
uniform sampler2D uRoughness;

uniform sampler2D uBRDFLut;
uniform samplerCube uEnvironment;
uniform samplerCube uIrradiance;
uniform sampler2D uShadowMap;

uniform sampler2D uReflections;
uniform sampler2D uReflectionsEmission;

in vec3 fNormal;
in vec3 fPosition;
in vec2 fUv;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oEmission;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0, float roughness);

void main() {


    float metallic = texture(uMetallic, fUv).r;
    float roughness = texture(uRoughness, fUv).r;
    
    vec3 normal = normalize((uModel * vec4(fNormal, 0.0)).xyz);
    vec3 worldPos = (uModel * vec4(fPosition, 1.0)).xyz;
    vec3 viewPos = (uView * vec4(worldPos, 1.0)).xyz;

    vec3 N = normalize(normal);
    vec3 V = normalize(uCameraPos - worldPos);
    vec3 L = normalize(uLightPos);
    vec3 H = normalize(V + L);
    vec3 R = normalize(reflect(-V, N));

    float NdotL = max(dot(N, L), 0.0);                
    float NdotV = max(dot(N, V), 0.0);
    
    vec3 albedo = (texture(uMap, fUv) * uColor).rgb;
        
    albedo.r = pow(albedo.r, 2.2);
    albedo.g = pow(albedo.g, 2.2);
    albedo.b = pow(albedo.b, 2.2);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 fragment = vec3(0.0);

    float NDF = DistributionGGX(N, H, roughness);        
    float G   = GeometrySmith(N, V, L, roughness);      
    //vec3 F    = fresnelSchlick(max(dot(N, V), 0.0), F0, roughness);       
    vec3 F    = fresnelSchlick(max(dot(N, H), 0.0), F0, roughness);       

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    vec3 radiance = vec3(uLightRadiance);
    vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 0.001);
    fragment += (kD * albedo / PI + specular) * radiance * NdotL; 
    
    // Irradiance
    #ifndef NO_INDIRECT_LIGHTING
    vec3 irradiance = texture(uIrradiance, N).rgb;
    fragment += (kD * irradiance * albedo);

    // Sky reflections
    ivec2 resolution = textureSize(uReflections, 0);
    vec3 reflections = texture(uReflections, gl_FragCoord.xy / resolution).rgb * F;
    vec3 reflectionsEmission = texture(uReflectionsEmission, gl_FragCoord.xy / resolution).rgb * F;
    fragment += reflections;

    if(length(reflections) == 0.0) { 
        vec2 envBrdf = texture(uBRDFLut, vec2(NdotV, roughness)).xy;
        vec3 indirectSpecular = texture(uEnvironment, R).rgb * (F * envBrdf.x + envBrdf.y);
        fragment += indirectSpecular;
    }

    #endif

    oColor = vec4(fragment, 1.0);
    oEmission = vec4(uEmission * albedo + reflectionsEmission, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0, float roughness)
{
    vec3 f = F0 + (1.0 - F0) * pow(1.0 - clamp(cosTheta, 0.0, 1.0), 5.0);
    return f * (1.0 - pow(roughness, 0.25));
} 
	