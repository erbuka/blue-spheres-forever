#pragma once

#include <fstream>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace bsf
{
	class Texture2D;


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



	#pragma endregion

	#pragma region Time

	struct Time
	{
		float Delta, Elapsed;
	};
	#pragma endregion

	#pragma region Ref

	template<typename T>
	using Ref = std::shared_ptr<T>;


	template<typename T, typename... Args>
	Ref<T> MakeRef(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	#pragma endregion

	#pragma region Utilities

	template<typename T>
	void Read(std::istream& is, T& target) {
		is.read((char*)&target, sizeof(T));
	}


	template<typename T>
	T Read(std::istream& is)
	{
		T value;
		is.read((char*)&value, sizeof(T));
		return value;
	}

	template<typename T>
	std::vector<T> Read(std::istream& is, uint32_t count) {
		std::vector<T> result(count);
		is.read((char*)result.data(), count * sizeof(T));
		return result;
	}

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


	uint32_t LoadProgram(const std::string& vertexSource, const std::string& fragmentSource);

	Ref<Texture2D> CreateCheckerBoard(const std::array<uint32_t, 2>& colors);

	#pragma endregion

}
