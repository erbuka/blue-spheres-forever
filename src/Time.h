#pragma once

namespace bsf
{
	struct Time
	{
		float Delta = 0.0f, Elapsed = 0.0f;

		inline Time& operator+=(const Time& other) { return operator+=(other.Delta); }
		inline Time& operator+=(float t) { Elapsed += t; return *this; }

		Time& operator-=(const Time& other) { return operator-=(other.Delta); }
		Time& operator-=(float t) { Elapsed -= t; return *this; }

	};
}

