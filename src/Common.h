#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <json/json.hpp>

#include "Log.h"
#include "Ref.h"

namespace glm
{
	using namespace nlohmann;

	// Matrices
	template<template<length_t, length_t, typename T, qualifier Q> typename M, length_t C, length_t R, typename T, qualifier Q>
	void to_json(json& json, const M<C, R, T, Q>& m)
	{
		const T* ptr = glm::value_ptr<T>(m);
		json = json::array();
		for (size_t i = 0; i < C * R; i++)
			json[i] = *(ptr + i);
	}

	template<template<length_t, length_t, typename T, qualifier Q> typename M, length_t C, length_t R, typename T, qualifier Q>
	void from_json(const json& json, M<C, R, T, Q>& m)
	{
		T* ptr = glm::value_ptr<T>(m);
		for (size_t i = 0; i < C * R; i++)
			*(ptr + i) = json[i].get<T>();
	}
	
	// Quaternions
	template<template<typename, glm::qualifier> typename V, typename T, qualifier Q>
	void to_json(json& json, const V<T, Q>& v)
	{
		const T* ptr = glm::value_ptr<T>(v);
		json = json::array();
		for (size_t i = 0; i < 4; i++)
			json[i] = *(ptr + i);
	}

	template<template<typename, glm::qualifier> typename V, typename T, glm::qualifier Q>
	void from_json(const json& j, V<T, Q>& v) {
		T* ptr = glm::value_ptr<T>(v);
		for (size_t i = 0; i < 4; i++)
			*(ptr + i) = j[i].get<T>();
	}

	// Vectors
	template<template<int, typename, glm::qualifier> typename V, int N, typename T, glm::qualifier Q>
	void to_json(json& json, const V<N, T, Q>& v)
	{
		const T* ptr = glm::value_ptr<T>(v);
		json = json::array();
		for (size_t i = 0; i < N; i++)
			json[i] = *(ptr + i);
	}

	template<template<int, typename, glm::qualifier> typename V, int N, typename T, glm::qualifier Q>
	void from_json(const json& j, V<N, T, Q>& v) {
		T* ptr = glm::value_ptr<T>(v);
		for (size_t i = 0; i < N; i++)
			*(ptr + i) = j[i].get<T>();
	}
}

namespace bsf
{
	class Texture2D;
	class VertexArray;
	

	#pragma region PBR

	struct Vertex3D
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 Uv;
	};
	
	constexpr float s_GroundRadius = 12.5f;
	constexpr int32_t s_SightRadius = 12;
	constexpr glm::vec3 s_GroundCenter = { 0.0f, 0.0f, -s_GroundRadius };
	
	std::tuple<bool, glm::vec3, glm::mat4> Reflect(const glm::vec3& cameraPosition, const glm::vec3& position, float radius);
	std::tuple<glm::vec3, glm::mat4> Project(const glm::vec3& position, bool invertOrientation = false);
	Ref<VertexArray> CreateClipSpaceQuad();
	Ref<VertexArray> CreateIcosphere(float radius, uint32_t recursion);
	Ref<VertexArray> CreateGround(int32_t left, int32_t right, int32_t bottom, int32_t top, int32_t divisions);
	Ref<VertexArray> CreateCube();
	std::vector<std::array<glm::vec3, 2>> CreateCubeData();



	#pragma endregion

	#pragma region Game Data Structures

	enum class GameMode
	{
		BlueSpheres, CustomStage
	};

	struct GameInfo
	{
		GameMode Mode;
		uint64_t Score;
		uint32_t CurrentStage;
	};

	#pragma endregion
	
	#pragma region Utilities

	struct GLEnableScope
	{
	public:
		GLEnableScope(const std::initializer_list<GLenum>& bits);
		~GLEnableScope();
	private:
		std::unordered_map<GLenum, GLboolean> m_SavedState;
	};


	bool Base64Decode(std::string_view str, std::vector<std::byte>& result);


	struct Rect
	{
		glm::vec2 Position = { 0.0f, 0.0f }, Size = { 0.0f, 0.0f };

		bool Contains(const glm::vec2& pos) const;
		bool Intersects(const Rect& other) const;
		Rect Intersect(const Rect& other) const;

		float Aspect() const { return Size.x / Size.y; }
		float InverseAspect() const { return 1.0f / Aspect(); }

		float Left() const { return Position.x; }
		float Right() const { return Position.x + Size.x; }
		float Bottom() const { return Position.y; }
		float Top() const { return Position.y + Size.y; }

		float Width() const { return Size.x; }
		float Height() const { return Size.y; }

		explicit operator glm::vec4() const {
			return { Left(), Right(), Bottom(), Top() };
		}

		void Shrink(float x, float y);
		void Shrink(float amount);

	};

	template<typename T>
	constexpr T Log2(T n)
	{
		//static_assert((T)n > 0 && !(n & (n - 1)));
		T result = 0;

		while ((n & T(1)) == 0)
		{
			++result;
			n >>= 1;
		}

		return result;

	}

	template <typename T>
	typename std::enable_if<std::is_unsigned<T>::value, int>::type
	constexpr Sign(T const x) 
	{
		return T(0) < x;
	}

	template <typename T>
	typename std::enable_if<std::is_signed<T>::value, int>::type
	constexpr Sign(T const x) {
		return (T(0) < x) - (x < T(0));
	}


	template<typename Enum, typename... Args>
	constexpr std::underlying_type_t<Enum> MakeFlags(Enum f, Args... args)
	{
		static_assert(std::is_enum_v<Enum> && std::is_integral_v<std::underlying_type_t<Enum>>);
		if constexpr (sizeof...(Args) > 0) return static_cast<std::underlying_type_t<Enum>>(f) | MakeFlags(args...);
		else return static_cast<std::underlying_type_t<Enum>>(f);
	}

	
	template<typename T, typename K>
	constexpr T MoveTowards(const T& from, const T& target, K k)
	{
		static_assert(std::is_arithmetic_v<T>, "T is not arithmetic");
		static_assert(std::is_arithmetic_v<K>, "K is not arithmetic");

		T distance = target - from;
		T dir = distance >= 0.0f ? 1.0f : -1.0f;
		return  k < std::abs(distance) ? from + k * dir : target;
	}

	template<template<int, typename, glm::qualifier> typename V, int N, typename T>
	constexpr V<N, T, glm::defaultp> MoveTowards(const V<N, T, glm::defaultp>& from, const V<N, T, glm::defaultp>& target, T t)
	{
		auto diff = target - from;
		T length = glm::length(diff);
		return  t < length ? from + t * glm::normalize(diff) : target;
	}

	template<typename ...Args>
	std::string Format(std::string_view format, Args... args)
	{
		static constexpr uint32_t bufSize = 1024;
		static char buffer[bufSize];
		std::snprintf(buffer, bufSize, format.data(), args...);
		return std::string(buffer);
	}


	void Trim(std::string& str);

	uint32_t UniqueId();


	template<typename T>
	struct InterpolatedValue
	{
	public:
		InterpolatedValue(): m_v0(0), m_v1(0), m_Value(0) {}

		void Reset(T v0, T v1)
		{
			m_v0 = v0;
			m_v1 = v1;
			m_Value = v0;
		}

		explicit operator T() const { return m_Value; }

		void operator()(float t) { m_Value = (1 - t) * m_v0 + t * m_v1; }

		template<uint8_t N>
		T Get() const;

		template<>
		T Get<0>() const { return m_v0; }

		template<>
		T Get<1>() const { return m_v1; }

	private:
		T m_Value;
		T m_v0, m_v1;
	};
	



	Ref<Texture2D> CreateCheckerBoard(const std::array<uint32_t, 2>& colors, Ref<Texture2D> target = nullptr);
	Ref<Texture2D> CreateGray(float value);
	Ref<Texture2D> CreateGradient(uint32_t size, const std::initializer_list<std::pair<float, glm::vec3>>& steps);

	std::string ReadTextFile(std::string_view file);


	#pragma endregion

}
