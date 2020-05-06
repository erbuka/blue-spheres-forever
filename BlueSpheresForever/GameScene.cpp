#include "GameScene.h"
#include "VertexArray.h"
#include "Common.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "GameLogic.h"
#include "Log.h"
#include "Assets.h"
#include "Renderer2D.h"
#include "CubeCamera.h"

#include <array>
#include <vector>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#pragma region Shaders

bool paused = false;

static const std::string s_TestVertex = R"Vertex(
	#version 330 core

	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;

	layout(location = 0) in vec3 aPosition;

	void main() {
		gl_Position =  uProjection * uView * uModel * vec4(aPosition, 1.0);;
	}	
)Vertex";

static const std::string s_TestFragment = R"Fragment(
	#version 330 core
	
	out vec4 oColor;
	
	void main() {
		oColor = vec4(1.0);
	}	
)Fragment";



static const std::string s_StarsVertex = R"Vertex(
	#version 330 core

	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;

	uniform vec2 uOffset;

	const float PI = 3.141592;

	layout(location = 0) in vec2 aUv;
	layout(location = 1) in vec3 aColor;
	layout(location = 2) in float aSize;
	
	flat out vec4 gPosition;
	flat out vec3 gColor;
	flat out float gSize;
	
	vec3 dome(in vec2 uv) {
		uv = uv * 2.0 - 1.0;

		float x = uv.x;
		float y = uv.y;		
		//float z = -(x*x + y*y) + 1.0;			
		float z = sqrt(1.0 - x*x - y*y);
		return vec3(x, y, z - 0.9);
			
	}	

	void main() {
	
		vec2 coords = fract(aUv + uOffset);
		vec4 position = uProjection * uView * uModel * vec4(dome(coords) * 10.0, 1.0);
		
		gPosition = position;
		gSize = aSize;
		gColor = aColor;

		gl_Position =  position;
	}	

)Vertex";

static const std::string s_StarsGeometry = R"Geometry(
	#version 330
	
	uniform vec2 uUnitSize;

	layout(points) in;
	layout(triangle_strip, max_vertices = 4) out;	

	flat in vec4 gPosition[1];
	flat in vec3 gColor[1];
	flat in float gSize[1];		
	
	flat out vec3 fColor;
	out vec2 fUv;

	void main() {
		vec4 position = gPosition[0] / gPosition[0].w;
		float sx = uUnitSize.x * gSize[0] * 2.0f;
		float sy = uUnitSize.y * gSize[0] * 2.0f;
			
		fColor = gColor[0];

		gl_Position = position + vec4(+sx, -sy, position.z, position.w); fUv = vec2(1.0f, 0.0f); EmitVertex();
		gl_Position = position + vec4(+sx, +sy, position.z, position.w); fUv = vec2(1.0f, 1.0f); EmitVertex();
		gl_Position = position + vec4(-sx, -sy, position.z, position.w); fUv = vec2(0.0f, 0.0f); EmitVertex();
		gl_Position = position + vec4(-sx, +sy, position.z, position.w); fUv = vec2(0.0f, 1.0f); EmitVertex();
		
		
		EndPrimitive();
		
	}

)Geometry";


static const std::string s_StarsFragment = R"Fragment(
	#version 330 core
	
	uniform sampler2D uMap;

	flat in vec3 fColor;
	in vec2 fUv;
	
	out vec4 oColor;

	void main() {
		oColor = texture(uMap, fUv) * vec4(fColor, 1.0);
	}


)Fragment";

static const std::string s_Vertex = R"Vertex(
	#version 330 core
	
	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;
	
	uniform vec2 uUvOffset;

	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec3 aNormal;
	layout(location = 2) in vec3 aTangent;
	layout(location = 3) in vec3 aBinormal;
	layout(location = 4) in vec2 aUv;

	
	out vec3 fPosition;
	out vec2 fUv;
	out mat3 fTBN;
	
	void main() {
		
		vec3 position = (uView * uModel * vec4(aPosition, 1.0)).xyz;

		gl_Position = uProjection * vec4(position, 1.0);

		// Compute TBN matrix
		vec3 normal = (uModel * vec4(aNormal, 0.0)).xyz;
		vec3 tangent = (uModel * vec4(aTangent, 0.0)).xyz;
		vec3 binormal = (uModel * vec4(aBinormal, 0.0)).xyz;
		fTBN = mat3(tangent, binormal, normal);

		fPosition = aPosition;
		fUv = aUv + uUvOffset;
	}	
)Vertex";


static const std::string s_Fragment = R"Fragment(
	#version 330 core
	
	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;
	
	uniform vec3 uCameraPos;
	uniform vec3 uLightPos;

	uniform vec4 uColor;

	uniform vec3 uSkyColor0;
	uniform vec3 uSkyColor1;

	uniform sampler2D uMap;
	uniform sampler2D uNormalMap;
	uniform sampler2D uMetallic;
	uniform sampler2D uRoughness;
	uniform sampler2D uAo;

	uniform sampler2D uBRDFLut;
	uniform samplerCube uEnvironment;
	
	in vec3 fPosition;
	in vec2 fUv;

	in mat3 fTBN;

	layout(location = 0) out vec4 oColor;
	layout(location = 1) out vec3 oNormal;
	layout(location = 2) out vec3 oPosition;

	const float PI = 3.14159265359;

	float DistributionGGX(vec3 N, vec3 H, float roughness);
	float GeometrySchlickGGX(float NdotV, float roughness);
	float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
	vec3 fresnelSchlick(float cosTheta, vec3 F0, float roughness);

	void main() {


		float metallic = texture(uMetallic, fUv).r;
		float roughness = texture(uRoughness, fUv).r;
		float ao = texture(uAo, fUv).r;
		
		//vec3 normal = fTBN * (texture(uNormalMap, fUv).xyz * 2.0 - 1.0);
		vec3 normal = fTBN[2];
		vec3 worldPos = (uModel * vec4(fPosition, 1.0)).xyz;
		vec3 viewPos = (uView * vec4(worldPos, 1.0)).xyz;

		vec3 N = normalize(normal);
		vec3 V = normalize(uCameraPos - worldPos);
		vec3 L = normalize(uLightPos - worldPos);
		vec3 H = normalize(V + L);
		vec3 R = normalize(reflect(-V, N));

		float distance = length(uCameraPos - worldPos);
		float attenuation = 1.0 / (distance * distance);

        float NdotL = max(dot(N, L), 0.0);                
		float NdotV = max(dot(N, V), 0.0);
		
		vec3 albedo = (texture(uMap, fUv) * uColor).rgb;
			
		albedo.r = pow(albedo.r, 2.2);
		albedo.g = pow(albedo.g, 2.2);
		albedo.b = pow(albedo.b, 2.2);

		vec3 F0 = mix(vec3(0.04), albedo, metallic);

        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(N, H), 0.0), F0, roughness);       
        

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL;
        vec3 specular     = numerator / max(denominator, 0.001);  
            
        // add to outgoing radiance Lo
		vec3 radiance = vec3(820.0, 810.0, 800.0) * attenuation;
		//vec3 radiance = vec3(10.0, 9.0, 10.0) * attenuation;
		//vec3 radiance = vec3(1.0) * attenuation;
        vec3 color = (kD * albedo / PI + specular) * radiance * NdotL;

		// sky reflections
		vec2 envBrdf = texture(uBRDFLut, vec2(NdotV, roughness)).xy;
		vec3 indirectSpecular = texture(uEnvironment, R).rgb * (F * envBrdf.x + envBrdf.y);
		vec3 ambient = (vec3(0.03) * albedo + indirectSpecular) * ao;

		vec3 fragment = ambient + color;

		fragment = fragment / (fragment + vec3(1.0));


		oColor = vec4(fragment, 1.0);
		oNormal = (uView * vec4(N, 0.0)).xyz * 0.5 + 0.5;
		oPosition = viewPos;
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

	
)Fragment";



static const std::string s_ScreenVertex = R"Vertex(
	#version 330 core
	
	layout(location = 0) in vec2 aPosition;

	out vec2 fUv;

	void main() {
		fUv = aPosition.xy * 0.5 + 0.5;
		gl_Position = vec4(aPosition, 0.0, 1.0);
	}
)Vertex";

static const std::string s_SkyGradientVertex = R"Vertex(
	#version 330 core

	uniform mat4 uProjection;
	uniform mat4 uView;
	
	layout(location = 0) in vec3 aPosition;

	out vec3 fPosition;

	void main() {
		fPosition = aPosition;
		gl_Position = uProjection * uView * vec4(aPosition, 1.0);
	}
)Vertex";

static const std::string s_SkyGradientFragment = R"Vertex(
	#version 330 core

	uniform vec3 uColor0;
	uniform vec3 uColor1;
	uniform vec2 uOffset;

	in vec3 fPosition;
	
	out vec4 oColor;

	void main() {
		
		oColor = vec4(mix(uColor1, uColor0, fPosition.y * 0.5 + 0.5), 1.0);
	}

)Vertex";

static const std::string s_DeferredFragment = R"Fragment(
	#version 330 core

	uniform mat4 uProjection;
	uniform mat4 uProjectionInv;

	uniform sampler2D uColor;
	uniform sampler2D uNormal;
	uniform sampler2D uPosition;

	in vec2 fUv;

	out vec4 oColor;	


	void main() {
		oColor = vec4(texture(uColor, fUv).rgb, 1.0);
	}
	
)Fragment";


static const std::string s_SkyBoxVertex = R"Vertex(
	#version 330 core

	uniform mat4 uProjection;	
	uniform mat4 uView;
	uniform mat4 uModel;

	layout(location = 0) in vec3 aPosition;

	out vec3 fPosition;

	void main() {
		gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
		fPosition = aPosition;
	}
	
)Vertex";

static const std::string s_SkyBoxFragment = R"Fragment(
	#version 330 core
	
	uniform samplerCube uSkyBox;

	in vec3 fPosition;

	layout(location = 0) out vec4 oColor;
	layout(location = 1) out vec3 oNormal;
	layout(location = 2) out vec3 oPosition;

	void main() {
		oColor = vec4(texture(uSkyBox, fPosition).rgb, 1.0);
		oNormal = vec3(0.0);
		oPosition = fPosition * 100.0;
	}
	
)Fragment";

static const std::string s_SkyPlaneVertex = R"Vertex(
	#version 330 core
	
	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;

	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec2 aUv;

	out vec2 fUv;

	void main() {
		fUv = aUv;
		gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
	}
		
)Vertex";

static const std::string s_SkyPlaneFragment = R"Fragment(
	#version 330 core
	
	uniform vec2 uOffset;
	uniform sampler2D uMap;

	in vec2 fUv;		
	
	out vec4 oColor;	

	void main() {
		oColor = vec4(texture(uMap, fUv - uOffset).rgb, 1.0);
	}
	
)Fragment";

#pragma endregion


namespace bsf
{

	struct Vertex
	{
		glm::vec3 Position0;
		glm::vec3 Normal0;
		glm::vec3 Tangent0;
		glm::vec3 Binormal0;
		glm::vec2 UV;
	};

	struct StarVertex
	{
		glm::vec2 UV;
		glm::vec3 Color;
		float Size;

	};

#pragma region Utilities


	static std::array<glm::vec3, 4>  Project(const glm::vec3& position)
	{
		constexpr float radius = 12.5f;
		constexpr glm::vec3 center = { 0.0f, 0.0f, -radius };

		float offset = position.z;
		glm::vec3 ground = { position.x, position.y, 0.0f };

		glm::vec3 normal = glm::normalize(ground - center);
		glm::vec3 tangent = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal);
		glm::vec3 binormal = glm::cross(normal, tangent);
		glm::vec3 pos = center + normal * (radius + offset);

		return { pos, normal, tangent, binormal };


	}


	static Ref<VertexArray> CreateSkyPlane(float size = 100.0f, uint32_t divisions = 50, float height = 4.0f)
	{
		struct SkyVertex {
			glm::vec3 Position;
			glm::vec2 Uv;
		};

		float step = size / divisions;

		std::vector<SkyVertex> vertices;

		vertices.reserve(divisions * divisions * 6);

		for (uint32_t y = 0; y < divisions; y++)
		{
			for (uint32_t x = 0; x < divisions; x++)
			{
				float x0 = x * step - size / 2.0f;
				float y0 = y * step - size / 2.0f;
				float x1 = (x + 1) * step - size / 2.0f;
				float y1 = (y + 1) * step - size / 2.0f;

				float u0 = float(x) / divisions;
				float u1 = float(x + 1) / divisions;
				float v0 = float(y) / divisions;
				float v1 = float(y + 1) / divisions;
					
				std::array<glm::vec3, 4> positions = {
					glm::vec3{ x0, y0, height },
					glm::vec3{ x1, y0, height },
					glm::vec3{ x1, y1, height },
					glm::vec3{ x0, y1, height }
				};

				std::array<glm::vec2, 4> uvs = {
					glm::vec2{ u0, v0 },
					glm::vec2{ u1, v0 },
					glm::vec2{ u1, v1 },
					glm::vec2{ u0, v1 }
				};

				std::array<SkyVertex, 4> v = { };

				for (int i = 0; i < 4; i++)
				{
					auto [pos, normal, tangent, binormal] = Project(positions[i]);
					v[i] = { pos, uvs[i] };
				}

				vertices.push_back(v[0]); vertices.push_back(v[2]); vertices.push_back(v[1]);
				vertices.push_back(v[0]); vertices.push_back(v[3]); vertices.push_back(v[2]);

			}

		}

		auto result = Ref<VertexArray>(new VertexArray({
			{ "aPosition", AttributeType::Float3 },
			{ "aUv", AttributeType::Float2 }
		}));

		result->SetData(vertices.data(), vertices.size(), GL_STATIC_DRAW);

		return result;

	}

	static Ref<VertexArray> CreateClipSpaceQuad()
	{
		auto result = Ref<VertexArray>(new VertexArray({
			{ "aPosition", AttributeType::Float2 }
		}));

		std::array<glm::vec2, 6> vertices = {
			glm::vec2(-1.0f, -1.0f),
			glm::vec2(1.0f, -1.0f),
			glm::vec2(1.0f,  1.0f),

			glm::vec2(-1.0f, -1.0f),
			glm::vec2(1.0f, 1.0f),
			glm::vec2(-1.0f, 1.0f)
		};

		result->SetData(vertices.data(), 6, GL_STATIC_DRAW);

		return result;
	}


	static Ref<VertexArray> CreateSkyDome(const Stage& stage)
	{
		uint32_t starCount = 1000;
		
		std::vector<StarVertex> stars(1000);

		for (uint32_t i = 0; i < starCount; i++)
		{
			stars[i].UV = { float(rand()) / RAND_MAX, float(rand()) / RAND_MAX };
			stars[i].Color = stage.StarColors[rand() % stage.StarColors.size()];
			stars[i].Size = (float(rand()) / RAND_MAX) < 0.1f ? 5.0f : 1.0f;
		}

		auto result = Ref<VertexArray>(new VertexArray({
			{ "aUV", AttributeType::Float2 },
			{ "aColor", AttributeType::Float3 },
			{ "aSize", AttributeType::Float }
		}));

		result->SetData(stars.data(), stars.size(), GL_STATIC_DRAW);

		return result;

		
	}

	static Ref<VertexArray> CreateIcosphere(float radius, uint32_t recursion)
	{
		const float t = (1.0f + glm::sqrt(5.0f)) / 2.0f;


		auto subdivide = [](const std::vector<glm::vec3>& v)
		{
			std::vector<glm::vec3> result;
			result.reserve(v.size() * 4);

			for (uint32_t i = 0; i < v.size(); i += 3)
			{
				const auto& a = v[i + 0];
				const auto& b = v[i + 1];
				const auto& c = v[i + 2];

				auto ab = glm::normalize((a + b) / 2.0f);
				auto bc = glm::normalize((b + c) / 2.0f);
				auto ac = glm::normalize((a + c) / 2.0f);

				result.push_back(a); result.push_back(ab); result.push_back(ac);
				result.push_back(ab); result.push_back(b); result.push_back(bc);
				result.push_back(ab); result.push_back(bc); result.push_back(ac);
				result.push_back(ac); result.push_back(bc); result.push_back(c);

			}

			return result;
		};

		std::array<glm::vec3, 12> v = {
			glm::normalize(glm::vec3(-1, t, 0)),
			glm::normalize(glm::vec3(1, t, 0)),
			glm::normalize(glm::vec3(-1, -t, 0)),
			glm::normalize(glm::vec3(1, -t, 0)),
			glm::normalize(glm::vec3(0, -1, t)),
			glm::normalize(glm::vec3(0, 1, t)),
			glm::normalize(glm::vec3(0, -1, -t)),
			glm::normalize(glm::vec3(0, 1, -t)),
			glm::normalize(glm::vec3(t, 0, -1)),
			glm::normalize(glm::vec3(t, 0, 1)),
			glm::normalize(glm::vec3(-t, 0, -1)),
			glm::normalize(glm::vec3(-t, 0, 1))
		};

		std::vector<glm::vec3> triangles = {
			v[0], v[11], v[5],
			v[0], v[5], v[1],
			v[0], v[1], v[7],
			v[0], v[7], v[10],
			v[0], v[10], v[11],
			v[1], v[5], v[9],
			v[5], v[11], v[4],
			v[11], v[10], v[2],
			v[10], v[7], v[6],
			v[7], v[1], v[8],
			v[3], v[9], v[4],
			v[3], v[4], v[2],
			v[3], v[2], v[6],
			v[3], v[6], v[8],
			v[3], v[8], v[9],
			v[4], v[9], v[5],
			v[2], v[4], v[11],
			v[6], v[2], v[10],
			v[8], v[6], v[7],
			v[9], v[8], v[1]
		};

		while (recursion-- > 0)
			triangles = subdivide(triangles);

		std::vector<Vertex> vertices;
		vertices.reserve(triangles.size());

		for (const auto& t : triangles)
		{
			float u = (std::atan2f(t.z, t.x) / (2.0f * glm::pi<float>()));
			float v = (std::asinf(t.y) / glm::pi<float>()) + 0.5f;
			vertices.push_back({ t * radius, glm::normalize(t), glm::vec3(), glm::vec3(), glm::vec2(u, v) });
		}

		Ref<VertexArray> result = Ref<VertexArray>(new VertexArray({
			{ "aPosition", AttributeType::Float3 },
			{ "aNormal", AttributeType::Float3 },
			{ "aTangent", AttributeType::Float3 },
			{ "aBinormal", AttributeType::Float3 },
			{ "aUv", AttributeType::Float2 }
		}));

		result->SetData(vertices.data(), vertices.size(), GL_STATIC_DRAW);

		return result;
	}

	static Ref<VertexArray> CreateWorld(int32_t left, int32_t right, int32_t bottom, int32_t top, int32_t divisions)
	{

		std::vector<Vertex> vertices;

		float step = 1.0f / divisions;

		for (int32_t x = left; x < right; x++)
		{
			for (int32_t y = bottom; y < top; y++)
			{
				for (int32_t dx = 0; dx < divisions; dx++)
				{
					for (int32_t dy = 0; dy < divisions; dy++)
					{
						float x0 = x + dx * step, y0 = y + dy * step;
						float x1 = x + (dx + 1) * step, y1 = y + (dy + 1) * step;
						float u0 = (x % 2) * 0.5f + dx * step * 0.5f, v0 = (y % 2) * 0.5f + dy * step * 0.5f;
						float u1 = (x % 2) * 0.5f + (dx + 1) * step * 0.5f, v1 = (y % 2) * 0.5f + (dy + 1) * step * 0.5f;
						
						std::array<glm::vec2, 4> coords = {
							glm::vec2{ x0, y0 },
							glm::vec2{ x1, y0 },
							glm::vec2{ x1, y1 },
							glm::vec2{ x0, y1 }
						};

						std::array<glm::vec2, 4> uvs = {
							glm::vec2(u0, v0),
							glm::vec2(u1, v0),
							glm::vec2(u1, v1),
							glm::vec2(u0, v1)
						};

						std::array<Vertex, 4> v = { };

						for (uint32_t i = 0; i < 4; i++)
						{
							auto [pos, normal, tangent, binormal] = Project(glm::vec3(coords[i], 0.0f));
							v[i] = { pos, normal, tangent, binormal, uvs[i] };
						}


						vertices.push_back(v[0]); vertices.push_back(v[1]); vertices.push_back(v[2]);
						vertices.push_back(v[0]); vertices.push_back(v[2]); vertices.push_back(v[3]);
					}
				}
			}
		}

		Ref<VertexArray> result = Ref<VertexArray>(new VertexArray({
			{ "aPosition", AttributeType::Float3 },
			{ "aNormal", AttributeType::Float3 },
			{ "aTangent", AttributeType::Float3 },
			{ "aBinormal", AttributeType::Float3 },
			{ "aUv", AttributeType::Float2 }
		}));

		result->SetData(vertices.data(), vertices.size(), GL_STATIC_DRAW);

		return result;

	}

	static Ref<VertexArray> CreateSkyBox()
	{

		std::array<glm::vec3, 8> v = {
			// Front
			glm::vec3(-1.0f, -1.0f, +1.0f),
			glm::vec3(+1.0f, -1.0f, +1.0f),
			glm::vec3(+1.0f, +1.0f, +1.0f),
			glm::vec3(-1.0f, +1.0f, +1.0f),

			// Back
			glm::vec3(-1.0f, -1.0f, -1.0f),
			glm::vec3(+1.0f, -1.0f, -1.0f),
			glm::vec3(+1.0f, +1.0f, -1.0f),
			glm::vec3(-1.0f, +1.0f, -1.0f),

		};

		std::vector<glm::vec3> verts;
		verts.reserve(36);

		// Front
		verts.push_back(v[0]); verts.push_back(v[2]); verts.push_back(v[1]);
		verts.push_back(v[0]); verts.push_back(v[3]); verts.push_back(v[2]);

		// Back
		verts.push_back(v[4]); verts.push_back(v[5]); verts.push_back(v[6]);
		verts.push_back(v[4]); verts.push_back(v[6]); verts.push_back(v[7]);

		// Left
		verts.push_back(v[0]); verts.push_back(v[4]); verts.push_back(v[7]);
		verts.push_back(v[0]); verts.push_back(v[7]); verts.push_back(v[3]);

		// Right
		verts.push_back(v[1]); verts.push_back(v[6]); verts.push_back(v[5]);
		verts.push_back(v[1]); verts.push_back(v[2]); verts.push_back(v[6]);

		// Top
		verts.push_back(v[3]); verts.push_back(v[6]); verts.push_back(v[2]);
		verts.push_back(v[3]); verts.push_back(v[7]); verts.push_back(v[6]);

		// Bottom
		verts.push_back(v[0]); verts.push_back(v[1]); verts.push_back(v[5]);
		verts.push_back(v[0]); verts.push_back(v[5]); verts.push_back(v[4]);


		Ref<VertexArray> result(new VertexArray({
			{ "aPosition", AttributeType::Float3 }
		}));

		result->SetData(verts.data(), verts.size(), GL_STATIC_DRAW);

		return result;
 
	}

#pragma endregion


	GameScene::GameScene(const Ref<Stage>& stage) :
		m_Stage(stage)
	{
	}

	void GameScene::OnAttach(Application& app)
	{
		auto windowSize = app.GetWindowSize();

		m_Subscriptions.push_back(app.WindowResized.Subscribe(this, &GameScene::OnResize));

		app.KeyPressed.Subscribe([&](const KeyPressedEvent& evt) {
			if (evt.KeyCode == GLFW_KEY_LEFT)
			{
				m_GameLogic->Rotate(GameLogic::ERotate::Left);
			}
			else if (evt.KeyCode == GLFW_KEY_RIGHT)
			{
				m_GameLogic->Rotate(GameLogic::ERotate::Right);
			}
			else if (evt.KeyCode == GLFW_KEY_UP)
			{
				m_GameLogic->RunForward();
			}
			else if (evt.KeyCode == GLFW_KEY_SPACE)
			{
				m_GameLogic->Jump();
			}
			else if (evt.KeyCode == GLFW_KEY_ENTER)
			{
				paused = !paused;
			}
		});

		m_GameLogic = MakeRef<GameLogic>(*m_Stage);


		// Renderer2D
		m_Renderer2D = MakeRef<Renderer2D>();

		// Framebuffers
		m_fbDeferred = MakeRef<Framebuffer>(windowSize.x, windowSize.y, true);
		m_fbDeferred->AddColorAttachment("color");
		m_fbDeferred->AddColorAttachment("normal", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
		m_fbDeferred->AddColorAttachment("position", GL_RGB32F, GL_RGB, GL_FLOAT);

		// Vertex arrays
		m_vaWorld = CreateWorld(-10, 10, -10, 10, 10);
		m_vaSphere = CreateIcosphere(0.15, 3);
		m_vaStars = CreateSkyDome(*m_Stage);
		m_vaQuad = CreateClipSpaceQuad();
		m_vaSkyBox = CreateSkyBox();
		m_vaSkyPlane = CreateSkyPlane();

		// Programs
		m_pPBR = MakeRef<ShaderProgram>(s_Vertex, s_Fragment);
		m_pStars = MakeRef<ShaderProgram>(s_StarsVertex, s_StarsGeometry, s_StarsFragment);
		m_pSkyGradient = MakeRef<ShaderProgram>(s_SkyGradientVertex, s_SkyGradientFragment);
		m_pDeferred = MakeRef<ShaderProgram>(s_ScreenVertex, s_DeferredFragment);
		m_pSkyBox = MakeRef<ShaderProgram>(s_SkyBoxVertex, s_SkyBoxFragment);
		m_pSkyPlane = MakeRef<ShaderProgram>(s_SkyPlaneVertex, s_SkyPlaneFragment);

		// Textures
		if (m_Stage->FloorRenderingMode == EFloorRenderingMode::CheckerBoard)
		{
			m_txGroundMap = CreateCheckerBoard({ ToHexColor(m_Stage->CheckerColors[0]), ToHexColor(m_Stage->CheckerColors[1]) });
			m_txGroundMap->Filter(TextureFilter::MagFilter, TextureFilterMode::Nearest);
			m_txGroundMap->Filter(TextureFilter::MinFilter, TextureFilterMode::Nearest);
		}
		else
		{
			m_txGroundMap = Ref<Texture2D>(new Texture2D((std::filesystem::path("assets/textures") / m_Stage->Texture).string()));
			m_txGroundMap->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			m_txGroundMap->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_txGroundMap->SetAnisotropy(16.0f);
		} 


		m_txEnv = Ref<TextureCube>(new TextureCube(
			"assets/textures/front.png",
			"assets/textures/back.png",
			"assets/textures/left.png",
			"assets/textures/right.png",
			"assets/textures/bottom.png",
			"assets/textures/top.png"
		));

		m_txClouds = MakeRef<Texture2D>("assets/textures/clouds.png");
		m_txClouds->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
		m_txClouds->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			

		// Ground normal map

		// Ground metallic/roughness/ao

		// Sky box camera
		m_ccSkyBox = MakeRef<CubeCamera>(2048);
	}
	
	void GameScene::OnRender(Application& app, const Time& time)
	{
		auto windowSize = app.GetWindowSize();
		float aspect = windowSize.x / windowSize.y;
		auto& assets = Assets::Get();

		glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 blue(0.0f, 0.0f, 1.0f, 1.0f);
		glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 yellow(1.0f, 1.0f, 0.0f, 1.0f);

		if (!paused)
		{
			const int steps = 5;
			for (int i = 0; i < steps; i++)
				m_GameLogic->Advance({ time.Delta / steps, time.Elapsed });
		}

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);

		glm::vec2 pos = m_GameLogic->GetPosition();
		int32_t ix = pos.x, iy = pos.y;
		float fx = pos.x - ix, fy = pos.y - iy;


		auto drawStageObject = [&](EStageObject val) {

			if (val != EStageObject::None)
			{
				m_pPBR->UniformTexture("uMap", assets.GetTexture(AssetName::TexWhite), 0);

				switch (val)
				{
				case EStageObject::RedSphere:
					assets.GetTexture(AssetName::TexWhite)->Bind(0);
					m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(red));
					m_vaSphere->Draw(GL_TRIANGLES);
					break;
				case EStageObject::BlueSphere:
					assets.GetTexture(AssetName::TexWhite)->Bind(0);
					m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(blue));
					m_vaSphere->Draw(GL_TRIANGLES);
					break;
				case EStageObject::YellowSphere:
					assets.GetTexture(AssetName::TexWhite)->Bind(0);
					m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(yellow));
					m_vaSphere->Draw(GL_TRIANGLES);
					break;
				case EStageObject::StarSphere:
					assets.GetTexture(AssetName::TexBumper)->Bind(0);
					m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(white));
					m_vaSphere->Draw(GL_TRIANGLES);
					break;
				}

			}
		};

		auto setupView = [&]() {
			// Setup the player view
			m_View.Reset();
			m_Model.Reset();

			m_View.LoadIdentity();
			m_Model.LoadIdentity();

			m_View.LookAt({ -1.5f, 2.5f, 0.0f }, { 1.0f, 0.0, 0.0f }, { 0.0f, 1.0f, 0.0f });
			m_View.Rotate({ 0.0f, 1.0f, 0.0f }, -m_GameLogic->GetRotationAngle());
			m_Model.Rotate({ 1.0f, 0.0f, 0.0f }, -glm::pi<float>() / 2.0f);
		};

		m_Projection.Reset();
		m_Projection.Perspective(glm::pi<float>() / 4.0f, aspect, 0.1f, 30.0f);

		// Render skybox
		{
			m_Model.Reset();
			m_View.Reset();
			GenerateSkyBox(pos, windowSize);
		}

		// Begin scene
		glViewport(0, 0, windowSize.x, windowSize.y);

		// Draw to deferred frame buffer
		m_fbDeferred->Bind();
		{

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			

			// Draw Sky
			
			{

				m_View.Reset();
				m_View.LoadIdentity();
				//m_View.LookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, -2.5f, -2.5f }, { 0.0f, 1.0f, 0.0f });
				m_View.LookAt({ 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0, 0.0f }, { 0.0f, 1.0f, 0.0f });
				//m_View.Rotate({ 0.0f, 1.0f, 0.0f }, -m_GameLogic->GetRotationAngle() + glm::pi<float>() / 2.0f);
				
				m_Model.Reset();
				m_Model.LoadIdentity();
				
				glDepthMask(GL_FALSE);
				m_pSkyBox->Use();
				m_pSkyBox->UniformMatrix4f("uProjection", m_Projection);
				m_pSkyBox->UniformMatrix4f("uView", m_View);
				m_pSkyBox->UniformMatrix4f("uModel", m_Model);
				m_pSkyBox->UniformTexture("uSkyBox", m_ccSkyBox->GetTexture(), 0);
				m_vaSkyBox->Draw(GL_TRIANGLES);
				glDepthMask(GL_TRUE);
			}
			

			// Draw scene
			/*
			{
				setupView();

				glm::vec3 cameraPosition = glm::inverse(m_View.GetMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				glm::vec3 lightVector = { 0.0f, 2.0f, 0.0f };

				// Draw ground
				
				m_pPBR->Use();

				m_pPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
				m_pPBR->UniformMatrix4f("uView", m_View.GetMatrix());
				m_pPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());

				m_pPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
				m_pPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightVector));

				m_pPBR->Uniform3fv("uSkyColor0", 1, glm::value_ptr(m_Stage->SkyColors[0]));
				m_pPBR->Uniform3fv("uSkyColor1", 1, glm::value_ptr(m_Stage->SkyColors[1]));

				m_pPBR->UniformTexture("uMap", m_txGroundMap, 0);
				m_pPBR->UniformTexture("uNormalMap", assets.GetTexture(AssetName::TexNormalPosZ), 1);
				m_pPBR->UniformTexture("uMetallic", assets.GetTexture(AssetName::TexGroundMetallic), 2);
				m_pPBR->UniformTexture("uRoughness", assets.GetTexture(AssetName::TexGroundRoughness), 3);
				m_pPBR->UniformTexture("uAo", assets.GetTexture(AssetName::TexWhite), 4);

				m_pPBR->UniformTexture("uBRDFLut", assets.GetTexture(AssetName::TexBRDFLut), 5);
				m_pPBR->UniformTexture("uEnvironment", m_ccSkyBox->GetTexture(), 6);
				

				m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(white));
				m_pPBR->Uniform2f("uUvOffset", { (ix % 2) * 0.5f + fx * 0.5f, (iy % 2) * 0.5f + fy * 0.5f });


				m_vaWorld->Draw(GL_TRIANGLES);


				// Draw player
				m_Model.Push();
				m_Model.Translate({ 0.0f, 0.0f, 0.15f + m_GameLogic->GetHeight() });
				m_pPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());
				m_pPBR->UniformTexture("uMap", assets.GetTexture(AssetName::TexWhite), 0);
				m_pPBR->Uniform2f("uUvOffset", { 0, 0 });
				m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(white));
				m_vaSphere->Draw(GL_TRIANGLES);
				m_Model.Pop();




				// Draw spheres and rings
				m_pPBR->UniformTexture("uMap", assets.GetTexture(AssetName::TexWhite), 0);
				m_pPBR->UniformTexture("uMetallic", assets.GetTexture(AssetName::TexSphereMetallic), 2); // Sphere metallic
				m_pPBR->UniformTexture("uRoughness", assets.GetTexture(AssetName::TexSphereRoughness), 3); // Sphere roughness
				m_pPBR->UniformTexture("uAo", assets.GetTexture(AssetName::TexWhite), 4); // Sphere ao

				for (int32_t x = -12; x <= 12; x++)
				{
					for (int32_t y = -12; y <= 12; y++)
					{
						m_Model.Push();
						m_Model.Translate(Project({ x - fx, y - fy, 0.15f })[0]);
						m_pPBR->UniformMatrix4f("uModel", m_Model);
						drawStageObject(m_Stage->GetValueAt(x + ix, y + iy));
						m_Model.Pop();
					}
				}

			

			}
			*/
		}
		m_fbDeferred->Unbind();

		// Draw to default frame buffer
		{
			glEnable(GL_FRAMEBUFFER_SRGB);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			m_pDeferred->Use();
				
			m_pDeferred->UniformMatrix4f("uProjection", m_Projection);
			m_pDeferred->UniformMatrix4f("uProjectionInv", glm::inverse(m_Projection.GetMatrix()));

			m_pDeferred->UniformTexture("uColor", m_fbDeferred->GetColorAttachment("color"), 0);
			m_pDeferred->UniformTexture("uNormal", m_fbDeferred->GetColorAttachment("normal"), 1);
			m_pDeferred->UniformTexture("uPosition", m_fbDeferred->GetColorAttachment("position"), 2);
			m_vaQuad->Draw(GL_TRIANGLES);
			
			glDisable(GL_FRAMEBUFFER_SRGB);


		}
		
	}
	
	void GameScene::OnDetach(Application& app)
	{
		for (auto& unsub : m_Subscriptions)
			unsub();
	}

	void GameScene::OnResize(const WindowResizedEvent& evt)
	{
		glViewport(0, 0, evt.Width, evt.Height);
		m_fbDeferred->Resize(evt.Width, evt.Height);
	}

	void GameScene::GenerateSkyBox(const glm::vec2& position, const glm::vec2& windowSize)
	{
		auto& assets = Assets::Get();

		static std::array<TextureCubeFace, 6> faces = {
			TextureCubeFace::Right,
			TextureCubeFace::Left,
			TextureCubeFace::Top,
			TextureCubeFace::Bottom,
			TextureCubeFace::Front,
			TextureCubeFace::Back
		};

		

		float u = 1.0f - position.x / m_Stage->GetWidth();
		float v = 1.0f - position.y / m_Stage->GetHeight();
		uint32_t ix = position.x;
		uint32_t iy = position.y;
		float fx = position.x - ix;
		float fy = position.y - iy;

		for (auto face : faces) {
			m_ccSkyBox->BindForRender(face);

			glClearColor(0, 0, 0, 1);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			// Test
			
			{
				float a = m_GameLogic->GetRotationAngle();

				m_Model.Reset();

				m_Model.Rotate({ 1.0f, 0.0, 0.0f }, -v * glm::pi<float>() * 2.0f);
				m_Model.Rotate({ 0.0f, 0.0, 1.0f }, -u * glm::pi<float>() * 2.0f);

				m_pSkyBox->Use();
				m_pSkyBox->UniformMatrix4f("uProjection", m_ccSkyBox->GetProjectionMatrix());
				m_pSkyBox->UniformMatrix4f("uView", m_ccSkyBox->GetViewMatrix());
				m_pSkyBox->UniformMatrix4f("uModel", m_Model);
				m_pSkyBox->UniformTexture("uSkyBox", m_txEnv, 0);
				m_vaSkyBox->Draw(GL_TRIANGLES);
			}
			

			// Draw sky plane
			


			/*
			{
				m_Model.Reset();
				m_Model.LoadIdentity();
				m_Model.Rotate({ 1.0f, 0.0f, 0.0f }, -glm::pi<float>() / 2.0f);

				m_pSkyPlane->Use();
				m_pSkyPlane->UniformTexture("uMap", m_txClouds, 0);
				m_pSkyPlane->UniformMatrix4f("uProjection", m_ccSkyBox->GetProjectionMatrix());
				m_pSkyPlane->UniformMatrix4f("uView", m_ccSkyBox->GetViewMatrix());
				m_pSkyPlane->UniformMatrix4f("uModel", m_Model);
				m_pSkyPlane->Uniform2f("uOffset", { u, v });
				m_vaSkyPlane->Draw(GL_TRIANGLES);
			}
			*/
			
			


			// Draw sky gradient
			/*
			{
				glDepthMask(GL_FALSE);
				m_pSkyGradient->Use();
				m_pSkyGradient->Uniform3fv("uColor0", 1, glm::value_ptr(m_Stage->SkyColors[0]));
				m_pSkyGradient->Uniform3fv("uColor1", 1, glm::value_ptr(m_Stage->SkyColors[1]));
				m_pSkyGradient->Uniform2f("uOffset", { u, v });
				m_pSkyGradient->UniformMatrix4f("uProjection", m_ccSkyBox->GetProjectionMatrix());
				m_pSkyGradient->UniformMatrix4f("uView", m_ccSkyBox->GetViewMatrix());
				m_vaSkyBox->Draw(GL_TRIANGLES);
				glDepthMask(GL_TRUE);
			}
			*/
			


			// Draw stars
			/*
			{

				m_Model.Reset();

				// It's going to be centered at the camera and drawn like a cubemap
				// so we just disable depth write

				float unit = 1.0f / 200.0f; // The base unit size of a star, relative to the screen width

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDepthMask(GL_FALSE);

				m_Model.LoadIdentity();
				//m_Model.LookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, -2.5f, -2.5f });
				//m_Model.Rotate({ 0.0f, 1.0f, 0.0f }, -m_GameLogic->GetRotationAngle() + glm::pi<float>() / 2.0f);
				m_Model.Rotate({ 1.0f, 0.0f, 0.0f }, -glm::pi<float>() / 2.0f);



				m_pStars->Use();
				m_pStars->UniformMatrix4f("uProjection", m_ccSkyBox->GetProjectionMatrix());
				m_pStars->UniformMatrix4f("uView", m_ccSkyBox->GetViewMatrix());
				m_pStars->UniformMatrix4f("uModel", m_Model.GetMatrix());
				m_pStars->UniformTexture("uMap", assets.GetTexture(AssetName::TexStar), 0);
				m_pStars->Uniform2f("uOffset", { u, v });
				m_pStars->Uniform2f("uUnitSize", { unit, unit });
				m_vaStars->Draw(GL_POINTS);

				m_Model.LoadIdentity();
				m_Model.Translate({ 1.0f, -0.4f, 0.0f });
				m_pTest->Use();
				m_pTest->UniformMatrix4f("uProjection", m_ccSkyBox->GetProjectionMatrix());
				m_pTest->UniformMatrix4f("uView", m_ccSkyBox->GetViewMatrix());
				m_pTest->UniformMatrix4f("uModel", m_Model.GetMatrix());
				m_vaSphere->Draw(GL_TRIANGLES);


				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);

			}
			*/
		}


	}
}