#include "BsfPch.h"
#include "StageCodeHelper.h"

namespace bsf
{
	StageCodeHelper::StageCodeHelper(uint64_t code)
	{
		uint64_t divider = std::pow(10, DigitCount - 1);
		for (uint8_t i = 0; i < DigitCount; i++)
		{
			m_Digits[i] = code / divider;
			assert(m_Digits[i] < 10);
			code %= divider;
			divider /= 10;
		}
	}

	uint8_t& StageCodeHelper::operator[](uint8_t index)
	{
		assert(index < DigitCount);
		return m_Digits[index];
	}

	const uint8_t& StageCodeHelper::operator[](uint8_t index) const
	{
		assert(index < DigitCount);
		return m_Digits[index];
	}

	StageCodeHelper::operator std::string() const
	{
		std::string result;
		for (uint8_t i = 0; i < DigitCount; i++)
		{
			if (i > 0 && i % 4 == 0)
				result += "-";

			result += std::to_string(m_Digits[i]);
		}
		return result;
	}

	StageCodeHelper::operator uint64_t() const
	{
		uint64_t result = 0;
		for (uint8_t i = 0; i < DigitCount; i++)
			result += std::pow(10, DigitCount - 1 - i) * m_Digits[i];
		return result;
	}
}