#pragma once

#include <functional>
#include <list>
#include "Log.h"

namespace bsf
{
	using Unsubscribe = std::function<void()>;

	#pragma region Events

	enum class Direction
	{
		Left, Right, Up, Down
	};

	enum class MouseButton : int32_t
	{
		None, Left, Right, Middle
	};

	struct MouseEvent
	{
		float X, Y, DeltaX, DeltaY;
		MouseButton Button;
	};

	struct WheelEvent
	{
		float DeltaX, DeltaY;
		float X, Y;
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

	struct CharacterTypedEvent
	{
		char Character;
	};
	
	#pragma endregion

	template<typename Event>
	class EventEmitter
	{
	public:

		using HandlerFn = std::function<void(const Event&)>;

		using HandlerFnPtr = void(*)(const Event&);

		template<typename T>
		using MemberHandlerFnPtr = void(T::*)(const Event&);

		EventEmitter() = default;
		EventEmitter(EventEmitter&) = delete;
		EventEmitter(EventEmitter&&) = delete;


		template<typename T>
		Unsubscribe Subscribe(T* instance, MemberHandlerFnPtr<T> fnPtr)
		{
			return Subscribe([instance, fnPtr](const Event& evt) { (instance->*fnPtr)(evt); });
		}

		Unsubscribe Subscribe(const HandlerFn& handler)
		{
			auto handlerIt = m_Handlers.insert(m_Handlers.end(), handler);
			return [&, handlerIt] { m_Handlers.erase(handlerIt); };
		}

		Unsubscribe Subscribe(HandlerFn&& handler)
		{
			auto handlerIt = m_Handlers.insert(m_Handlers.end(), std::move(handler));
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
				BSF_ERROR("Subscriber has been destroyed but still has {0} subscriptions", m_Subscriptions.size());
		}

		void ClearSubscriptions() {
			for (auto& unsub : m_Subscriptions)
				unsub();
			m_Subscriptions.clear();
		}

		template<typename Event>
		void AddSubscription(EventEmitter<Event>& evt, typename EventEmitter<Event>::HandlerFn&& handler) { 
			m_Subscriptions.push_back(evt.Subscribe(std::forward<EventEmitter<Event>::HandlerFn>(handler)));
		}
		
		
		template<typename Event, typename T>
		void AddSubscription(EventEmitter<Event>& evt, T* instance, typename EventEmitter<Event>::MemberHandlerFnPtr<T> handler)
		{
			m_Subscriptions.push_back(evt.Subscribe(instance, handler));
		}
		

	private:
		std::list<Unsubscribe> m_Subscriptions;

	};



}