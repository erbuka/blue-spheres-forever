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
#include "Config.h"

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
		virtual bool OnMouseMoved(MenuRoot& root, const glm::vec2& pos) { return false; }
		virtual bool OnMouseClicked(MenuRoot& root, const glm::vec2& pos) { return false; }
		virtual void Render(MenuRoot& root, Renderer2D& renderer) = 0;
	};

	class MenuItem : public MenuNode
	{
	public:
		bool Selected = false;
		Rect Bounds;

		virtual float GetHeight() const = 0;
		virtual std::string GetCaption() const = 0;

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

		virtual std::string GetCaption() const override { return Caption; }
		virtual float GetHeight() const override { return 1.0f; };

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

		virtual std::string GetCaption() const override { return Caption; }
		virtual float GetHeight() const override { return 1.0f; };

	private:
		ConfirmFn m_ConfirmFn = nullptr;
	};

	template<typename T>
	class SelectMenuItem : public MenuItem
	{
	public:

		std::string Caption;

		SelectMenuItem(const std::string& caption);

		bool OnConfirm(MenuRoot& root) override;
		bool OnDirectionInput(MenuRoot& root, Direction direction) override;

		void Render(MenuRoot& root, Renderer2D& renderer) override;

		void AddOption(const std::string& caption, const T& value);
		const T& GetSelectedOption();
		void SetSelectedOption(const T& val);

		virtual std::string GetCaption() const override { return m_Options[m_SelectedOption].first; }
		virtual float GetHeight() const override { return 1.5f; };


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
		bool OnMouseMoved(MenuRoot& root, const glm::vec2& pos) override;
		void Render(MenuRoot& root, Renderer2D& renderer) override;
		uint64_t GetStageCode() const { return m_CurrentCode; }

		virtual std::string GetCaption() const override { return (std::string)m_CurrentCode; }
		virtual float GetHeight() const override { return 1.5f; };


	private:
		uint32_t m_CursorPos;
		StageCodeHelper m_CurrentCode, m_PreviousCode;
		bool m_Input = false;
	};

	class Menu : public MenuNode
	{
	public:
		template<typename T, typename ...Args>
		Ref<std::enable_if_t<std::is_base_of_v<MenuItem, T>, T>> AddItem(Args&&... args)
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
		bool OnMouseMoved(MenuRoot& root, const glm::vec2& pos) override;
		bool OnMouseClicked(MenuRoot& root, const glm::vec2& pos) override;
		void Render(MenuRoot& root, Renderer2D& renderer) override;
		const std::vector<Ref<MenuItem>>& GetChildren() const { return m_Children; }

	private:
		Ref<MenuItem> m_SelectedItem = nullptr;
		std::vector<Ref<MenuItem>> m_Children;
	};

	class MenuRoot
	{
	public:
		glm::vec2 ViewportSize = { 0.0f, 0.0f };
		glm::vec2 WindowSize = { 0.0f, 0.0f };

		void FireOnConfirm();
		void FireOnDirectionInput(Direction direction);
		void FireOnKeyTyped(int32_t keyCode);
		void FireOnMouseMoved(const glm::vec2& pos);
		void FireOnMouseClicked(const glm::vec2& pos);

		void PushMenu(const Ref<Menu>& menu);
		void PopMenu();

		void Render(Renderer2D& renderer);

		Ref<Menu>& GetCurrentMenu();

	private:

		glm::vec2 WindowToViewport(const glm::vec2& pos) const;

		std::stack<Ref<Menu>> m_MenuStack;
	};

	class MenuScene : public Scene
	{
	public:

		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;


	private:
		void BuildMenus();
		void PlayStage(const Ref<Stage>& stage, const GameInfo& gameInfo);

		Config m_Config;
		MenuRoot m_MenuRoot;
		Ref<SelectMenuItem<std::string>> m_SelectStageMenuItem;
		Ref<SelectMenuItem<DisplayModeDescriptor>> m_DisplayModeMenuItem;
		Ref<SelectMenuItem<bool>> m_FullscreenMenuItem;
		Ref<StageCodeMenuItem> m_StageCodeMenuItem;
		Ref<Texture2D> m_txBackground;

	};


}

