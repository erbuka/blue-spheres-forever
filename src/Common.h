#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "Log.h"
#include "Ref.h"

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


	struct MorphVertex3D
	{
		glm::vec3 Position0, Position1;
		glm::vec3 Normal0, Normal1;
		glm::vec2 Uv0, Uv1;
	};
	
	constexpr float s_GroundRadius = 12.5f;
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

	enum class ByteOrder
	{
		BigEndian,
		LittleEndian
	};

	static struct 
	{
		uint32_t x = 1;
		uint8_t* p = reinterpret_cast<uint8_t*>(&x);
	} s_ByteOrderHelper;

	/// Retrieves the local system's byte order
	constexpr ByteOrder SystemByteOrder() {
		return *(s_ByteOrderHelper.p) == 1 ? ByteOrder::LittleEndian : ByteOrder::BigEndian;
	}

	template<typename T>
	void SwapBytes(std::enable_if_t<std::is_arithmetic_v<T>, T>& value)
	{
		
		constexpr uint32_t count = sizeof(value);
		uint8_t* base = reinterpret_cast<uint8_t*>(&value);

		for (int32_t i = 0; i < count / 2; i++)
			std::swap(*(base + i), *(base + count - (i + 1)));
	}


	template<ByteOrder B>
	class InputStream
	{
	public:
		InputStream(std::istream& source) : m_Source(source) {}

		// Generic read function. Doesn't care about endianness
		template<typename T>
		void Read(T& target) { 
			m_Source.read((char*)&target, sizeof(T)); 
			BSF_DEBUG("Read {0} bytes", sizeof(T));
		}

		// This gets enabled if the system byte order is different than the InputStream byte order
		// It's defined for numeric types
		template<typename T>
		void Read(std::enable_if_t<B != SystemByteOrder() && std::is_arithmetic_v<T>, T>& target)
		{
			m_Source.read((char*)&target, sizeof(T)); 
			SwapBytes(target);
			BSF_DEBUG("Read {0} bytes with byte order inversion", sizeof(T));
		}

		// Special case for glm vectors
		template<template<int, typename, glm::qualifier> typename V, int N, typename T, glm::qualifier Q>
		void Read(V<N, T, Q>& target)
		{
			for (uint32_t i = 0; i < N; i++)
				Read(*(glm::value_ptr(target) + i));
		}

		template<typename T>
		T Read() {
			T result;
			Read(result);
			return result;
		}

		template<typename T>
		void ReadSome(uint32_t count, T* ptr)
		{
			for (uint32_t i = 0; i < count; i++)
				Read(*(ptr + i));
		}



	private:
		std::istream& m_Source;
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
		inline constexpr Sign(T const x) 
	{
		return T(0) < x;
	}

	template <typename T>
	typename std::enable_if<std::is_signed<T>::value, int>::type
		inline constexpr Sign(T const x) {
		return (T(0) < x) - (x < T(0));
	}

	template<typename Enum>
	constexpr std::underlying_type_t<Enum> MakeFlags(Enum f)
	{
		return static_cast<std::underlying_type_t<Enum>>(f);
	}

	template<typename Enum, typename... Args>
	constexpr std::underlying_type_t<Enum> MakeFlags(Enum f, Args... args)
	{
		static_assert(std::is_enum_v<Enum> && std::is_integral_v<std::underlying_type_t<Enum>>);
		return static_cast<std::underlying_type_t<Enum>>(f) | MakeFlags(args...);
	}

	
	template<typename T, typename K>
	T MoveTowards(const T& from, const T& target, K k)
	{
		static_assert(std::is_arithmetic_v<T>, "T is not arithmetic");
		static_assert(std::is_arithmetic_v<K>, "K is not arithmetic");

		T distance = target - from;
		T dir = distance >= 0.0f ? 1.0f : -1.0f;
		return  k < std::abs(distance) ? from + k * dir : target;
	}

	template<template<int, typename, glm::qualifier> typename V, int N, typename T>
	V<N, T, glm::defaultp> MoveTowards(const V<N, T, glm::defaultp>& from, const V<N, T, glm::defaultp>& target, T t)
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
