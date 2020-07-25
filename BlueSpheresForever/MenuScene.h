#pragma once

#include <stack>
#include <vector>
#include <list>

#include "Scene.h"
#include "Common.h"
#include "EventEmitter.h"

namespace bsf
{

	class Renderer2D;
	class MenuRoot;
	class Menu;

	class MenuNode
	{
	public:
		virtual bool OnConfirm(MenuRoot& root) = 0;
		virtual bool OnDirectionInput(MenuRoot& root, Direction direction) = 0;
		virtual void Render(MenuRoot& root, Renderer2D& renderer) = 0;
	};

	class MenuItem : public MenuNode
	{
	public:
		bool Selected = false;
	};

	class MenuLinkItem : public MenuItem
	{
	public:

		std::string Caption;
		Ref<Menu> LinkedMenu = nullptr;
		
		MenuLinkItem(const std::string& caption, const Ref<Menu>& linkedMenu);

		bool OnConfirm(MenuRoot& root) override;
		bool OnDirectionInput(MenuRoot& root, Direction direction) override { return false; }
		void Render(MenuRoot& root, Renderer2D& renderer) override;
	};


	class MenuButtonItem : public MenuItem
	{
	public:
		using ConfirmFn = std::function<bool(MenuRoot&)>;

		std::string Caption;

		MenuButtonItem(const std::string& caption);

		void SetConfirmFunction(const ConfirmFn& fn) { m_ConfirmFn = fn; }

		bool OnConfirm(MenuRoot& root) override;
		bool OnDirectionInput(MenuRoot& root, Direction direction) override { return false; }
		void Render(MenuRoot& root, Renderer2D& renderer) override;
	private:
		ConfirmFn m_ConfirmFn = nullptr;
	};


	class Menu : public MenuNode
	{
	public:
		template<typename T, typename ...Args>
		Ref<std::enable_if_t<std::is_base_of_v<MenuItem, T>, T>> AddItem(Args... args)
		{
			auto item = MakeRef<T>(std::forward<Args>(args)...);
			m_Children.push_back(item);
			if (m_SelectedItem == nullptr)
				m_SelectedItem = item;
			return item;
		}

		void ResetSelected();
		bool OnConfirm(MenuRoot& root) override;
		bool OnDirectionInput(MenuRoot& root, Direction direction) override;
		void Render(MenuRoot& root, Renderer2D& renderer) override;

	private:
		Ref<MenuItem> m_SelectedItem = nullptr;
		std::vector<Ref<MenuItem>> m_Children;
	};

	class MenuRoot
	{
	public:
		glm::vec2 ViewportSize;

		void OnConfirm();

		void OnDirectionInput(Direction direction);

		void PushMenu(const Ref<Menu>& menu);
		void PopMenu();

		void Render(Renderer2D& renderer);

		Ref<Menu>& GetCurrentMenu();

	private:
		std::stack<Ref<Menu>> m_MenuStack;
	};

	class MenuScene : public Scene
	{
	public:
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;
	private:
		MenuRoot m_MenuRoot;
		std::list<Unsubscribe> m_Subscriptions;
	};

}

