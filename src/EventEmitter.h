#pragma once

#include <functional>
#include <list>
#include "Log.h"

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
			return Subscribe([fnPtr](const Event& evt) { fnPtr(evt); });
		}

		template<typename T>
		inline Unsubscribe Subscribe(T* instance, MemberHandlerFnPtr<T> fnPtr)
		{
			return Subscribe([instance, fnPtr](const Event& evt) { (instance->*fnPtr)(evt); });
		}

		Unsubscribe Subscribe(const HandlerFn& handler)
		{
			auto handlerIt = m_Handlers.insert(m_Handlers.end(), handler);
			return [&, handlerIt] { m_Handlers.erase(handlerIt); };
		}

		void Emit(const Event& evt)
		{
			for (auto& handler : m_Handlers)
				handler(evt);
		}

	protected:
		std::list<HandlerFn> m_Handlers;
	};

	class EventReceiver
	{
	public:
		virtual ~EventReceiver() { 
			if (!m_Subscriptions.empty())
				BSF_ERROR("Subscriber has been destroyed but still has subscriptions");
		}

		void ClearSubscriptions() {
			for (auto& unsub : m_Subscriptions)
				unsub();
			m_Subscriptions.clear();
		}

		template<typename Event>
		void AddSubscription(EventEmitter<Event>& evt, const typename EventEmitter<Event>::HandlerFn& handler) { 
			m_Subscriptions.push_back(evt.Subscribe(handler));
		}

	private:
		std::list<Unsubscribe> m_Subscriptions;

	};



}