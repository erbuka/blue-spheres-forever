#pragma once

#include <stack>
#include <vector>
#include <list>

#include "Ref.h"
#include "Common.h"
#include "Scene.h"
#include "EventEmitter.h"
#include "StageCodeHelper.h"
#include "MatrixStack.h"

namespace bsf
{

	class Renderer2D;
	class MenuRoot;
	class Menu;
	class Stage;
	class Sky;
	class Framebuffer;
	class ShaderProgram;

	class MenuNode
	{
	public:
		virtual bool OnConfirm(MenuRoot& root) = 0;
		virtual bool OnDirectionInput(MenuRoot& root, Direction direction) = 0;
		virtual bool OnKeyTyped(MenuRoot& root, int32_t keyCode) { return false; }
		virtual void Render(MenuRoot& root, Renderer2D& renderer) = 0;
		virtual float GetUIHeight() const { return 1.0f; }
	};

	class MenuItem : public MenuNode
	{
	public:
		bool Selected = false;
	};


	class LinkMenuItem : public MenuItem
	{
	public:

		std::string Caption;
		Ref<Menu> LinkedMenu = nullptr;
		
		LinkMenuItem(const std::string& caption, const Ref<Menu>& linkedMenu);

		bool OnConfirm(MenuRoot& root) override;
		bool OnDirectionInput(MenuRoot& root, Direction direction) override { return false; }
		void Render(MenuRoot& root, Renderer2D& renderer) override;
	};


	class ButtonMenuItem : public MenuItem
	{
	public:
		using ConfirmFn = std::function<bool(MenuRoot&)>;

		std::string Caption;

		ButtonMenuItem(const std::string& caption);

		void SetConfirmFunction(const ConfirmFn& fn) { m_ConfirmFn = fn; }

		bool OnConfirm(MenuRoot& root) override;
		bool OnDirectionInput(MenuRoot& root, Direction direction) override { return false; }
		void Render(MenuRoot& root, Renderer2D& renderer) override;
	private:
		ConfirmFn m_ConfirmFn = nullptr;
	};

	template<typename T>
	class SelectMenuItem : public MenuItem
	{
	public:

		std::string Caption;

		SelectMenuItem(const std::string& caption);

		bool OnConfirm(MenuRoot& root) override { return false; }
		bool OnDirectionInput(MenuRoot& root, Direction direction) override;
		void Render(MenuRoot& root, Renderer2D& renderer) override;
		float GetUIHeight() const override { return 1.5f; }

		void AddOption(const std::string& caption, const T& value);
		const T& GetSelectedOption();

	private:
		uint32_t m_SelectedOption;
		std::vector<std::pair<std::string, T>> m_Options;
	};

	class StageCodeMenuItem : public MenuItem
	{
	public:

		StageCodeMenuItem();

		bool OnConfirm(MenuRoot& root) override;
		bool OnDirectionInput(MenuRoot& root, Direction direction) override;
		bool OnKeyTyped(MenuRoot& root, int32_t keyCode) override;
		void Render(MenuRoot& root, Renderer2D& renderer) override;
		float GetUIHeight() const override { return 1.5f; }
		uint64_t GetStageCode() const { return m_CurrentCode; }

	private:
		uint32_t m_CursorPos;
		StageCodeHelper m_CurrentCode, m_PreviousCode;
		bool m_Input = false;
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
		bool OnKeyTyped(MenuRoot& root, int32_t keyCode) override;
		void Render(MenuRoot& root, Renderer2D& renderer) override;

	private:
		Ref<MenuItem> m_SelectedItem = nullptr;
		std::vector<Ref<MenuItem>> m_Children;
	};

	class MenuRoot
	{
	public:
		glm::vec2 ViewportSize = { 0.0f, 0.0f };

		void OnConfirm();
		void OnDirectionInput(Direction direction);
		void OnKeyTyped(int32_t keyCode);

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

		MenuScene(const Ref<Sky>& sky = nullptr) : m_Sky(sky) {}

		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;

		void OnResize(const WindowResizedEvent& evt);

	private:
		void BuildMenus();
		void DrawTitle(Renderer2D& r2);
		void PlayStage(const Ref<Stage>& stage, const GameInfo& gameInfo);

		MenuRoot m_MenuRoot;
		Ref<SelectMenuItem<std::string>> m_SelectStageMenuItem;
		Ref<StageCodeMenuItem> m_StageCodeMenuItem;
		Ref<Sky> m_Sky;
		Ref<Framebuffer> m_fbSky;
		Ref<ShaderProgram> m_pDeferred, m_pSky;
		MatrixStack m_Projection, m_View, m_Model;
	};


}

