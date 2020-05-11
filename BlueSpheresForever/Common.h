#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <algorithm>
#include <glad/glad.h>

#include "Log.h"

namespace bsf
{
	class Texture2D;
	class VertexArray;
	
	#pragma region Ref

	template<typename T>
	using Ref = std::shared_ptr<T>;


	template<typename T, typename... Args>
	Ref<T> MakeRef(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	#pragma endregion

	#pragma region Shaders

	enum class ShaderType
	{
		Vertex,
		Geometry,
		Fragment
	};

	struct ShaderSource
	{
		ShaderType Type;
		std::string Source;
	};

	#pragma endregion

	#pragma region PBR

	struct Vertex
	{
		glm::vec3 Position0;
		glm::vec3 Normal0;
		glm::vec3 Tangent0;
		glm::vec3 Binormal0;
		glm::vec2 UV;
	};

	using Model = std::vector<Ref<VertexArray>>;

	#pragma endregion

	#pragma region Events

	enum class MouseButton : int
	{
		None, Left, Right
	};

	struct MouseEvent
	{
		float X, Y;
		MouseButton Button;
	};

	struct WindowResizedEvent
	{
		float Width, Height;
	};

	struct KeyPressedEvent
	{
		int32_t KeyCode;
		bool Repeat;
	};

	struct KeyReleasedEvent
	{
		int32_t KeyCode;
	};
	
	struct GLEnableScope
	{
	public:
		GLEnableScope(const std::initializer_list<GLenum>& bits);
		~GLEnableScope();
	private:
		std::unordered_map<GLenum, GLboolean> m_SavedState;
	};



	#pragma endregion

	#pragma region Time

	struct Time
	{
		float Delta = 0.0f, Elapsed = 0.0f;

		Time& operator+=(const Time & other);

	};


	#pragma endregion



	#pragma region Utilities

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
			BSF_INFO("Read {0} bytes", sizeof(T));
		}

		// This gets enabled if the system byte order is different than the InputStream byte order
		// It's defined for numeric types
		template<typename T>
		void Read(std::enable_if_t<B != SystemByteOrder() && std::is_arithmetic_v<T>, T>& target)
		{
			m_Source.read((char*)&target, sizeof(T)); 
			SwapBytes(target);
			BSF_INFO("Read {0} bytes with byte order inversion", sizeof(T));
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


	template <typename T>
	typename std::enable_if<std::is_unsigned<T>::value, int>::type
		inline constexpr Sign(T const x) {
		return T(0) < x;
	}

	template <typename T>
	typename std::enable_if<std::is_signed<T>::value, int>::type
		inline constexpr Sign(T const x) {
		return (T(0) < x) - (x < T(0));
	}

	template<typename InputIt, typename BinOp, typename R, typename T>
	R Reduce(InputIt first, InputIt last, R init, BinOp op)
	{
		return R(0);
	}


	uint32_t LoadProgram(const std::initializer_list<ShaderSource>& shaderSources);

	uint32_t ToHexColor(const glm::vec3& rgb);
	uint32_t ToHexColor(const glm::vec4& rgb);

	Ref<Texture2D> CreateCheckerBoard(const std::array<uint32_t, 2>& colors);
	Ref<Texture2D> CreateGray(float value);

	#pragma endregion

}
