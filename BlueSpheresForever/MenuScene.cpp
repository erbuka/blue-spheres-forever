#include "BsfPch.h"

#include <glm/ext.hpp>

#include "MenuScene.h"
#include "Renderer2D.h"
#include "Application.h"
#include "Assets.h"
#include "Font.h"

namespace bsf
{
	static constexpr float s_VirtualHeight = 10;

	static constexpr glm::vec4 s_SelectedMenuColor = { 1.0f, 1.0f, 0.0f, 1.0f };
	static constexpr glm::vec4 s_MenuColor = { 1.0f, 1.0f, 1.0f, 1.0f };


	void MenuScene::OnAttach()
	{
		// Create menu
		auto mainMenu = MakeRef<Menu>();
		auto playMenu = MakeRef<Menu>();
		auto customStagesMenu = MakeRef<Menu>();

		auto backFn = [&](MenuRoot& root) { root.PopMenu(); return true; };
		
		playMenu->AddItem<MenuButtonItem>("Play");
		playMenu->AddItem<MenuButtonItem>("Back")->SetConfirmFunction(backFn);

		customStagesMenu->AddItem<MenuButtonItem>("Back")->SetConfirmFunction(backFn);

		mainMenu->AddItem<MenuLinkItem>("Play", playMenu);
		mainMenu->AddItem<MenuLinkItem>("Custom Stages", customStagesMenu);
		mainMenu->AddItem<MenuButtonItem>("Exit")->SetConfirmFunction([&](MenuRoot& root) { GetApplication().Exit(); return true; });

		m_MenuRoot.PushMenu(mainMenu);

		// Events
		auto& app = GetApplication();
		m_Subscriptions.push_back(app.KeyPressed.Subscribe([&](const KeyPressedEvent& evt) {
			switch (evt.KeyCode)
			{
			case GLFW_KEY_LEFT: m_MenuRoot.OnDirectionInput(Direction::Left); break;
			case GLFW_KEY_RIGHT: m_MenuRoot.OnDirectionInput(Direction::Right); break;
			case GLFW_KEY_UP: m_MenuRoot.OnDirectionInput(Direction::Up); break;
			case GLFW_KEY_DOWN: m_MenuRoot.OnDirectionInput(Direction::Down); break;
			case GLFW_KEY_ENTER: m_MenuRoot.OnConfirm(); break;
			default:
				break;
			}
		}));


		// Fade In


		ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, 
			MakeRef<FadeTask>(glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 1.0f, 0.0f }, 1.0f));

	}

	void MenuScene::OnRender(const Time& time)
	{

		auto& renderer2d = GetApplication().GetRenderer2D();
		auto windowSize = GetApplication().GetWindowSize();

		float height = s_VirtualHeight;
		float width = windowSize.x / windowSize.y * s_VirtualHeight;

		{
			GLEnableScope scope({ GL_DEPTH_TEST });

			glDisable(GL_DEPTH_TEST);

			glViewport(0, 0, windowSize.x, windowSize.y);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			renderer2d.Begin(glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f));
			m_MenuRoot.ViewportSize = { width, height };
			m_MenuRoot.Render(GetApplication().GetRenderer2D());
			renderer2d.End();
		}
	}

	void MenuScene::OnDetach()
	{
		for (auto& unsubscribe : m_Subscriptions)
			unsubscribe();
	}

	void MenuRoot::OnConfirm()
	{
		assert(m_MenuStack.size() > 0);
		m_MenuStack.top()->OnConfirm(*this);
	}

	void MenuRoot::OnDirectionInput(Direction direction)
	{
		assert(m_MenuStack.size() > 0);
		m_MenuStack.top()->OnDirectionInput(*this, direction);
	}

	void MenuRoot::PushMenu(const Ref<Menu>& menu)
	{
		m_MenuStack.push(menu);
	}

	void MenuRoot::PopMenu()
	{
		assert(m_MenuStack.size() > 0);
		m_MenuStack.pop();
	}

	void MenuRoot::Render(Renderer2D& renderer)
	{
		assert(m_MenuStack.size() > 0);
		renderer.Push();
		renderer.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
		renderer.Translate({ ViewportSize.x / 2.0f, ViewportSize.y / 2.0f });
		renderer.Pivot(EPivot::Center);
		m_MenuStack.top()->Render(*this, renderer);
		renderer.Pop();
	}

	Ref<Menu>& MenuRoot::GetCurrentMenu()
	{
		assert(m_MenuStack.size() > 0);
		return m_MenuStack.top();
	}



	void Menu::ResetSelected()
	{
		m_SelectedItem = m_Children.size() > 0 ? m_Children.front() : nullptr;
	}

	bool Menu::OnConfirm(MenuRoot& root)
	{
		assert(m_SelectedItem != nullptr);
		return m_SelectedItem->OnConfirm(root);
	}

	bool Menu::OnDirectionInput(MenuRoot& root, Direction direction)
	{
		assert(m_SelectedItem != nullptr);

		bool handled = m_SelectedItem->OnDirectionInput(root, direction);

		if (!handled)
		{
			auto iter = std::find(m_Children.begin(), m_Children.end(), m_SelectedItem);

			switch (direction)
			{
			case Direction::Up:
				if(iter != m_Children.begin())
					--iter;
				break;
			case Direction::Down:
				++iter;
				break;
			default:
				break;
			}

			m_SelectedItem = iter != m_Children.end() ? *iter : m_SelectedItem;

		}
		
		return true;

	}

	void Menu::Render(MenuRoot& root, Renderer2D& renderer)
	{
		renderer.Push();
		for (auto& child : m_Children)
		{
			child->Selected = m_SelectedItem == child;
			child->Render(root, renderer);
			renderer.Translate({ 0.0f, -1.0f });
		}
		renderer.Pop();
	}

	MenuLinkItem::MenuLinkItem(const std::string& caption, const Ref<Menu>& linkedMenu) :
		Caption(caption), LinkedMenu(linkedMenu)
	{
	}

	bool MenuLinkItem::OnConfirm(MenuRoot& root)
	{
		assert(LinkedMenu != nullptr);
		LinkedMenu->ResetSelected();
		root.PushMenu(LinkedMenu);
		return true;
	}

	void MenuLinkItem::Render(MenuRoot& root, Renderer2D& renderer)
	{
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		renderer.Color(Selected ? s_SelectedMenuColor : s_MenuColor);
		renderer.DrawString(font, Caption);
	}

	MenuButtonItem::MenuButtonItem(const std::string& caption) :
		Caption(caption)
	{
	}

	bool MenuButtonItem::OnConfirm(MenuRoot& root)
	{
		if (m_ConfirmFn)
			return m_ConfirmFn(root);
		return false;
	}

	void MenuButtonItem::Render(MenuRoot& root, Renderer2D& renderer)
	{
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		renderer.Color(Selected ? s_SelectedMenuColor : s_MenuColor);
		renderer.DrawString(font, Caption);
	}

}
