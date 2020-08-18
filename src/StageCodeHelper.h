#pragma once

#include <inttypes.h>
#include <array>
#include <string>

namespace bsf
{
	class StageCodeHelper
	{
	public:

		static constexpr uint8_t DigitCount = 12;

		StageCodeHelper() : StageCodeHelper(0) {}
		StageCodeHelper(uint64_t code);

		uint8_t& operator[](uint8_t index);
		const uint8_t& operator[](uint8_t index) const;

		operator uint64_t() const;

		explicit operator std::string() const;

	private:

		std::array<uint8_t, 12> m_Digits;
	};
}
