#pragma once

#include <functional>


namespace bsf
{
	using Unsubscribe = std::function<void()>;

	template<typename Event>
	class EventEmitter
	{
	public:

		using HandlerFn = std::function<void(const Event&)>;

		using HandlerFnPtr = void(*)(const Event&);

		template<typename T>
		using MemberHandlerFnPtr = void(T::*)(const Event&);

		EventEmitter() {}
		EventEmitter(EventEmitter&) = delete;
		EventEmitter(EventEmitter&&) = delete;

		inline Unsubscribe Subscribe(HandlerFnPtr fnPtr)
		{
			auto x = [fnPtr](const Event& evt) { fnPtr(evt); };

			return Subscribe(std::bind(fnPtr, std::placeholders::_1));
		}

		template<typename T>
		inline Unsubscribe Subscribe(T* instance, MemberHandlerFnPtr<T> fnPtr)
		{
			return Subscribe(std::bind(fnPtr, instance, std::placeholders::_1));
		}

		Unsubscribe Subscribe(const HandlerFn& handler)
		{
			auto handlerIt = m_Handlers.insert(m_Handlers.end(), handler);
			return [&, handlerIt] { m_Handlers.erase(handlerIt); };
		}

		void Emit(const Event& evt)
		{
			for (auto& handler : m_Handlers)
			{
				handler(evt);
			}
		}

	protected:
		std::list<HandlerFn> m_Handlers;
	};
}