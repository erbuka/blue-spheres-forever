#include "BsfPch.h"

#include "Common.h"
#include "Color.h"
#include "Log.h"
#include "Texture.h";
#include "VertexArray.h"

namespace bsf
{

	bool Rect::Contains(const glm::vec2& pos) const
	{
		const auto& min = Position;
		auto max = Position + Size;
		return pos.x >= min.x && pos.x <= max.x && pos.y >= min.y && pos.y <= max.y;
	}

	bool Rect::Intersects(const Rect& other) const
	{
		return glm::all(glm::greaterThan(Intersect(other).Size, glm::vec2(0.0f, 0.0f)));
	}

	Rect Rect::Intersect(const Rect& other) const
	{
		auto min = glm::max(Position, other.Position);
		auto max = glm::min(Position + Size, other.Position + other.Size);
		auto size = glm::max(max - min, { 0.0f, 0.0f });
		return { min, size };
	}

	void Rect::Shrink(float x, float y)
	{
		Position += glm::vec2(x, y);
		Size -= 2.0f * glm::vec2(x, y);
	}

	void Rect::Shrink(float amount)
	{
		Position += glm::vec2(amount);
		Size -= 2.0f * glm::vec2(amount);
	}


	Ref<Texture2D> CreateGray(float value) {
		return MakeRef<Texture2D>(ToHexColor({ value, value, value, 1.0 }));
	}

	Ref<Texture2D> CreateGradient(uint32_t size, const std::initializer_list<std::pair<float, glm::vec3>>& steps)
	{
		std::vector<std::pair<float, glm::vec3>> sortedSteps;
		std::vector<uint32_t> pixels(size);
		std::copy(steps.begin(), steps.end(), std::back_inserter(sortedSteps));

		for (uint32_t i = 0; i < size; ++i)
		{
			float t = (float)i / (size - 1);

			uint32_t j = 0;
			while (sortedSteps[j].first <= t && j < sortedSteps.size() - 1)
				++j;

			auto color = glm::lerp(sortedSteps[j - 1].second, sortedSteps[j].second,
				(t - sortedSteps[j - 1].first) / (sortedSteps[j].first - sortedSteps[j - 1].first));

			pixels[i] = ToHexColor(color);

		}


		auto result = MakeRef<Texture2D>(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
		result->SetFilter(TextureFilter::Linear, TextureFilter::Linear);
		result->SetPixels(pixels.data(), size, 1);
		return result;

	}

	std::string ReadTextFile(std::string_view file)
	{
		std::ifstream is;

		is.open(file);

		if (!is.is_open()) {
			BSF_ERROR("Can't open file: {0}", file);
			return "";
		}

		std::stringstream ss;

		ss << is.rdbuf();

		is.close();

		return ss.str();

	}


	/*
		TODO Review this. It sucks it's so ugly, but I don't like the version
		with find_first_of etc. That doesn't seem very safe.
		Have to reverse 2 times because I can't erase with reverse iterators
	*/
	void Trim(std::string& str)
	{
		static auto trimImpl = [](std::string& str) {
			for (auto it = str.begin(); it != str.end();)
			{
				if (*it == ' ')
					it = str.erase(it);
				else
					break;
			}
		};
		trimImpl(str);
		std::reverse(str.begin(), str.end());
		trimImpl(str);
		std::reverse(str.begin(), str.end());

	}

	uint32_t UniqueId()
	{
		/* 
		Did this just for fun, it should be fine for its purpose (generate file names),
		but It's not reliable in general. Don't use for complex ids
		*/
		static constexpr uint32_t mask = 0xabcede18;
		static uint32_t iteration = 0;
		using namespace std::chrono;
		uint32_t millis = (uint32_t)(duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count()
			+ (iteration++);

		millis ^= mask;

		return millis;
	}

	Ref<Texture2D> CreateCheckerBoard(const std::array<uint32_t, 2>& colors, Ref<Texture2D> target)
	{
		std::array<uint32_t, 4> data = {
			colors[0], colors[1],
			colors[1], colors[0]
		};

		if (target == nullptr)
			target = MakeRef<Texture2D>(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);

		target->SetPixels(data.data(), 2, 2);
		target->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
		return target;

	}


	std::tuple<bool, glm::vec3, glm::mat4> Reflect(const glm::vec3& cameraPosition, const glm::vec3& position, float radius)
	{
		static auto calcHorizon = [](const glm::vec3& obs) -> float {
			auto v = obs - s_GroundCenter;
			float h = glm::length(v) - s_GroundRadius;
			return glm::sqrt(h * (2.0f * s_GroundRadius + h));
		};

		float horizon = calcHorizon(cameraPosition);
		
		auto top = std::get<0>(Project(position + glm::vec3(0.0f, 0.0f, radius)));
		float maxTopDist = calcHorizon(top) + horizon;
		float topDist = glm::length(top - cameraPosition);
		float factor = glm::max((topDist - horizon) / (maxTopDist - horizon), 0.0f);

		return factor >= 1.0f ? std::make_tuple(false, glm::vec3(), glm::mat4()) :
			std::tuple_cat(
				std::make_tuple(true), 
				Project({ position.x, position.y, glm::lerp(-position.z, 2.0f * radius + position.z,  factor * factor) }, true));


	}

	std::tuple<glm::vec3, glm::mat4> Project(const glm::vec3& position, bool invertOrientation)
	{

		float offset = position.z;
		glm::vec3 ground = { position.x, position.y, 0.0f };

		glm::vec3 normal = glm::normalize(ground - s_GroundCenter);
		glm::vec3 tangent = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal);
		glm::vec3 binormal = glm::cross(normal, tangent);
		glm::vec3 pos = s_GroundCenter + normal * (s_GroundRadius + offset);

		glm::mat4 tbn = glm::identity<glm::mat4>();

		tbn[0] = { tangent * (invertOrientation ? -1.0f : 1.0f), 0.0f };
		tbn[1] = { binormal, 0.0f };
		tbn[2] = { normal * (invertOrientation ? -1.0f : 1.0f), 0.0f };

		return { pos, tbn };

	}

	Ref<VertexArray> CreateClipSpaceQuad()
	{

		std::array<glm::vec2, 6> vertices = {
			glm::vec2(-1.0f, -1.0f),
			glm::vec2(1.0f, -1.0f),
			glm::vec2(1.0f,  1.0f),

			glm::vec2(-1.0f, -1.0f),
			glm::vec2(1.0f, 1.0f),
			glm::vec2(-1.0f, 1.0f)
		};


		auto vb = Ref<VertexBuffer>(new VertexBuffer({
			{ "aPosition", AttributeType::Float2 }
			}, vertices.data(), vertices.size()));

		return Ref<VertexArray>(new VertexArray(vertices.size(), { vb }));

	}

	Ref<VertexArray> CreateIcosphere(float radius, uint32_t recursion)
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

		std::vector<Vertex3D> vertices;
		vertices.reserve(triangles.size());

		for (const auto& t : triangles)
		{
			float u = (std::atan2f(t.z, t.x) / (2.0f * glm::pi<float>()));
			float v = (std::asinf(t.y) / glm::pi<float>()) + 0.5f;
			vertices.push_back({ t * radius, glm::normalize(t), glm::vec2(u, v) });
		}

		auto vb = Ref<VertexBuffer>(new VertexBuffer({
			{ "aPosition", AttributeType::Float3 },
			{ "aNormal", AttributeType::Float3 },
			{ "aUv", AttributeType::Float2 }
			}, vertices.data(), vertices.size()));


		return Ref<VertexArray>(new VertexArray(vertices.size(), { vb }));
	}

	Ref<VertexArray> CreateGround(int32_t left, int32_t right, int32_t bottom, int32_t top, int32_t divisions)
	{

		std::vector<Vertex3D> vertices;

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

						std::array<Vertex3D, 4> v = { };

						for (uint32_t i = 0; i < 4; i++)
						{
							auto [pos, tbn] = Project(glm::vec3(coords[i], 0.0f));
							v[i] = { pos, glm::vec3(tbn[2]), uvs[i] };
						}


						vertices.push_back(v[0]); vertices.push_back(v[1]); vertices.push_back(v[2]);
						vertices.push_back(v[0]); vertices.push_back(v[2]); vertices.push_back(v[3]);
					}
				}
			}
		}

		auto vb = Ref<VertexBuffer>(new VertexBuffer({
			{ "aPosition", AttributeType::Float3 },
			{ "aNormal", AttributeType::Float3 },
			{ "aUv", AttributeType::Float2 }
			}, vertices.data(), vertices.size()));

		return Ref<VertexArray>(new VertexArray(vertices.size(), { vb }));

	}

	std::vector<std::array<glm::vec3, 2>> CreateCubeData()
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

		std::vector<std::array<glm::vec3, 2>> verts; // { position, uvw }
		verts.reserve(36);

		// Front
		verts.push_back({ v[0], v[0] }); verts.push_back({ v[2], v[2] }); verts.push_back({ v[1], v[1] });
		verts.push_back({ v[0], v[0] }); verts.push_back({ v[3], v[3] }); verts.push_back({ v[2], v[2] });
		// Back
		verts.push_back({ v[4], v[4] }); verts.push_back({ v[5], v[5] }); verts.push_back({ v[6], v[6] });
		verts.push_back({ v[4], v[4] }); verts.push_back({ v[6], v[6] }); verts.push_back({ v[7], v[7] });
		// Left
		verts.push_back({ v[0], v[0] }); verts.push_back({ v[4], v[4] }); verts.push_back({ v[7], v[7] });
		verts.push_back({ v[0], v[0] }); verts.push_back({ v[7], v[7] }); verts.push_back({ v[3], v[3] });
		// Right
		verts.push_back({ v[1], v[1] }); verts.push_back({ v[6], v[6] }); verts.push_back({ v[5], v[5] });
		verts.push_back({ v[1], v[1] }); verts.push_back({ v[2], v[2] }); verts.push_back({ v[6], v[6] });
		// Top
		verts.push_back({ v[3], v[3] }); verts.push_back({ v[6], v[6] }); verts.push_back({ v[2], v[2] });
		verts.push_back({ v[3], v[3] }); verts.push_back({ v[7], v[7] }); verts.push_back({ v[6], v[6] });
		// Bottom
		verts.push_back({ v[0], v[0] }); verts.push_back({ v[1], v[1] }); verts.push_back({ v[5], v[5] });
		verts.push_back({ v[0], v[0] }); verts.push_back({ v[5], v[5] }); verts.push_back({ v[4], v[4] });
		
		return verts;

	}

	bool Base64Decode(std::string_view str, std::vector<std::byte>& result)
	{
		static constexpr uint8_t kDecodingTable[] = {
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
		  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
		  64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
		  64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
		};

		size_t length = str.size();
		if (length % 4 != 0) return false;


		result.reserve(length / 4 * 3);
		size_t padding = (size_t)(str[length - 1] == '=') + (size_t)(str[length - 2] == '=');

		for (size_t i = 0; i < length; i += 4)
		{
			uint32_t data =
				(kDecodingTable[static_cast<uint32_t>(str[i + 0])] << 18) |
				(kDecodingTable[static_cast<uint32_t>(str[i + 1])] << 12) |
				(kDecodingTable[static_cast<uint32_t>(str[i + 2])] << 6) |
				(kDecodingTable[static_cast<uint32_t>(str[i + 3])] << 0);

			result.push_back(static_cast<std::byte>((data & 0xff0000) >> 16));
			result.push_back(static_cast<std::byte>((data & 0x00ff00) >> 8));
			result.push_back(static_cast<std::byte>((data & 0x0000ff) >> 0));

		}

		while (padding-- > 0)
			result.pop_back();
		
		return true;
	}

	Ref<VertexArray> CreateCube()
	{
		auto vertices = CreateCubeData();

		auto vb = Ref<VertexBuffer>(new VertexBuffer({
			{ "aPosition", AttributeType::Float3 },
			{ "aUv", AttributeType::Float3 },
			}, vertices.data(), vertices.size()));


		return Ref<VertexArray>(new VertexArray(vertices.size(), { vb }));

	}

	GLEnableScope::GLEnableScope(const std::initializer_list<GLenum>& bits)
	{
		GLboolean value;

		for (auto bit : bits)
		{
			BSF_GLCALL(glGetBooleanv(bit, &value));
			m_SavedState[bit] = value;
		}
	}

	GLEnableScope::~GLEnableScope()
	{
		for (const auto& bit : m_SavedState)
			bit.second ? glEnable(bit.first) : glDisable(bit.first);
	}

}

