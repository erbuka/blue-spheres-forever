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
	class Stage;

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

		void AddOption(const std::string& caption, const Ref<T>& value);
		const Ref<T>& GetSelectedOption();

	private:
		uint32_t m_SelectedOption;
		std::vector<std::pair<std::string, Ref<T>>> m_Options;
	};

	class StageCodeMenuItem : public MenuItem
	{
	public:

		class StageCode
		{
		public:

			static constexpr uint8_t DigitCount = 12;

			StageCode() : StageCode(0) {}
			StageCode(uint64_t code);

			uint8_t& operator[](uint8_t index);
			const uint8_t& operator[](uint8_t index) const;

			operator uint64_t() const;

		private:

			std::array<uint8_t, 12> m_Digits;
		};

		StageCodeMenuItem();

		bool OnConfirm(MenuRoot& root) override;
		bool OnDirectionInput(MenuRoot& root, Direction direction) override;
		bool OnKeyTyped(MenuRoot& root, int32_t keyCode) override;
		void Render(MenuRoot& root, Renderer2D& renderer) override;
		float GetUIHeight() const override { return 1.5f; }
		uint64_t GetStageCode() const { return m_CurrentCode; }

	private:
		uint32_t m_CursorPos;
		StageCode m_CurrentCode, m_PreviousCode;
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
		glm::vec2 ViewportSize;

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
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;
	private:

		std::vector<Ref<Stage>> LoadCustomStages();


		void PlayStage(const Ref<Stage>& stage);

		MenuRoot m_MenuRoot;
		Ref<SelectMenuItem<Stage>> m_SelectStageMenuItem;
		Ref<StageCodeMenuItem> m_StageCodeMenuItem;
		std::list<Unsubscribe> m_Subscriptions;
	};


}

