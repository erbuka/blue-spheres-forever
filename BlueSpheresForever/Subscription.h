#pragma once

#include <functional>

namespace bsf
{
	class Subscription
	{
	public:

		using UnsubscribeFn = std::function<void()>;

		Subscription(const UnsubscribeFn& unsub) : m_Unsubscribe(unsub), m_Valid(true) {}
		Subscription(Subscription&) = delete;
		Subscription(Subscription&&) = delete;

		bool IsValid() const { return m_Valid; }

		void Unsubscribe();

	private:
		bool m_Valid;
		UnsubscribeFn m_Unsubscribe;
	};
}