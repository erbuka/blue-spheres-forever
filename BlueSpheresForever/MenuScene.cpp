#include "BsfPch.h"

#include <glm/ext.hpp>
#include <sstream>

#include "MenuScene.h"
#include "Renderer2D.h"
#include "Application.h"
#include "Assets.h"
#include "Font.h"
#include "Stage.h"
#include "GameScene.h"
#include "Audio.h"

namespace bsf
{
	static constexpr float s_VirtualHeight = 10;

	static constexpr glm::vec4 s_SelectedMenuColor = { 1.0f, 1.0f, 0.0f, 1.0f };
	static constexpr glm::vec4 s_MenuColor = { 1.0f, 1.0f, 1.0f, 1.0f };


	template<typename T>
	SelectMenuItem<T>::SelectMenuItem(const std::string& caption) :
		Caption(caption), m_SelectedOption(0)
	{
	}

	template<typename T>
	bool SelectMenuItem<T>::OnDirectionInput(MenuRoot& root, Direction direction)
	{
		uint32_t newOption = m_SelectedOption;
		switch (direction)
		{
		case Direction::Left:
			newOption = newOption > 0 ? newOption - 1 : newOption;
			break;
		case Direction::Right:
			newOption = newOption < m_Options.size() - 1 ? newOption + 1 : newOption;
			break;
		default:
			return false;
			break;
		}

		m_SelectedOption = newOption;
	}

	template<typename T>
	void SelectMenuItem<T>::Render(MenuRoot& root, Renderer2D& renderer)
	{
		auto color = Selected ? s_SelectedMenuColor : s_MenuColor;
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);

		renderer.Push();
		renderer.Scale({ 0.5f, 0.5f });
		renderer.Color(color);
		renderer.DrawString(font, Caption);
		renderer.Translate({ 0.0f, -1.0f });
		renderer.Scale({ 2.0f, 2.0f });
		renderer.DrawString(font, m_Options[m_SelectedOption].first);
		renderer.Pop();
	}

	template<typename T>
	void SelectMenuItem<T>::AddOption(const std::string& caption, const Ref<T>& value)
	{
		m_Options.push_back({ caption, value });
	}

	template<typename T>
	const Ref<T>& SelectMenuItem<T>::GetSelectedOption()
	{
		return m_Options[m_SelectedOption].second;
	}

	void MenuScene::OnAttach()
	{

		// Create menus
		auto mainMenu = MakeRef<Menu>();
		auto playMenu = MakeRef<Menu>();
		auto customStagesMenu = MakeRef<Menu>();

		auto backFn = [&](MenuRoot& root) { root.PopMenu(); return true; };
		
		playMenu->AddItem<ButtonMenuItem>("Play")->SetConfirmFunction([&](MenuRoot& root) {
			auto code = m_StageCodeMenuItem->GetStageCode();
			auto stage = Assets::GetInstance().Get<StageGenerator>(AssetName::StageGenerator)->Generate(code);
			PlayStage(stage);
			return true;
		});
		m_StageCodeMenuItem = playMenu->AddItem<StageCodeMenuItem>();
		playMenu->AddItem<ButtonMenuItem>("Back")->SetConfirmFunction(backFn);


		customStagesMenu->AddItem<ButtonMenuItem>("Play")->SetConfirmFunction([&](MenuRoot& root) {
			auto stage = m_SelectStageMenuItem->GetSelectedOption();
			PlayStage(stage);
			return true;
		});

		{ // Custom stage select
			m_SelectStageMenuItem = customStagesMenu->AddItem<SelectMenuItem<Stage>>("Stage");

			uint32_t i = 0;
			for (const auto& stage : LoadCustomStages())
			{
				std::stringstream ss;
				ss << "Stage #" << ++i;
				m_SelectStageMenuItem->AddOption(ss.str(), stage);
			}
		}
		customStagesMenu->AddItem<ButtonMenuItem>("Back")->SetConfirmFunction(backFn);


		mainMenu->AddItem<LinkMenuItem>("Play", playMenu);
		mainMenu->AddItem<LinkMenuItem>("Custom Stages", customStagesMenu);
		mainMenu->AddItem<ButtonMenuItem>("Exit")->SetConfirmFunction([&](MenuRoot& root) { GetApplication().Exit(); return true; });

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
				if (!evt.Repeat)
					m_MenuRoot.OnKeyTyped(evt.KeyCode);
				break;
			}
		}));


		// Fade In
		ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, 
			MakeRef<FadeTask>(glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 1.0f, 0.0f }, 0.5f));

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

	std::vector<Ref<Stage>> MenuScene::LoadCustomStages()
	{
		namespace fs = std::filesystem;

		std::vector<Ref<Stage>> result;

		for (auto& entry : fs::directory_iterator("assets/data"))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".bss")
			{
				BSF_INFO("Loading stage: {0}", entry.path().string());
				auto stage = MakeRef<Stage>();
				if(stage->FromFile(entry.path().string()))
					result.push_back(stage);
			}
		}

		return result;
	}

	void MenuScene::PlayStage(const Ref<Stage>& stage)
	{
		auto fadeTask = MakeRef<FadeTask>(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f);

		Assets::GetInstance().Get<Audio>(AssetName::SfxIntro)->Stop();

		fadeTask->SetDoneFunction([&, stage](SceneTask& self) {
			GetApplication().GotoScene(MakeRef<GameScene>(stage));
		});

		ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, fadeTask);
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

	void MenuRoot::OnKeyTyped(int32_t keyCode)
	{
		assert(m_MenuStack.size() > 0);
		m_MenuStack.top()->OnKeyTyped(*this, keyCode);
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

	bool Menu::OnKeyTyped(MenuRoot& root, int32_t keyCode)
	{
		return m_SelectedItem->OnKeyTyped(root, keyCode);
	}

	void Menu::Render(MenuRoot& root, Renderer2D& renderer)
	{
		renderer.Push();
		for (auto& child : m_Children)
		{
			child->Selected = m_SelectedItem == child;
			child->Render(root, renderer);
			renderer.Translate({ 0.0f, -child->GetUIHeight() });
		}
		renderer.Pop();
	}

	LinkMenuItem::LinkMenuItem(const std::string& caption, const Ref<Menu>& linkedMenu) :
		Caption(caption), LinkedMenu(linkedMenu)
	{
	}

	bool LinkMenuItem::OnConfirm(MenuRoot& root)
	{
		assert(LinkedMenu != nullptr);
		LinkedMenu->ResetSelected();
		root.PushMenu(LinkedMenu);
		return true;
	}

	void LinkMenuItem::Render(MenuRoot& root, Renderer2D& renderer)
	{
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		renderer.Color(Selected ? s_SelectedMenuColor : s_MenuColor);
		renderer.DrawString(font, Caption);
	}

	ButtonMenuItem::ButtonMenuItem(const std::string& caption) :
		Caption(caption)
	{
	}

	bool ButtonMenuItem::OnConfirm(MenuRoot& root)
	{
		if (m_ConfirmFn)
			return m_ConfirmFn(root);
		return false;
	}

	void ButtonMenuItem::Render(MenuRoot& root, Renderer2D& renderer)
	{
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		renderer.Color(Selected ? s_SelectedMenuColor : s_MenuColor);
		renderer.DrawString(font, Caption);
	}


	
	StageCodeMenuItem::StageCodeMenuItem() : m_CursorPos(0)
	{
		auto generator = Assets::GetInstance().Get<StageGenerator>(AssetName::StageGenerator);
		m_CurrentCode = generator->GetCodeFromLevel(1);
	}

	bool StageCodeMenuItem::OnConfirm(MenuRoot& root)
	{
		m_Input = !m_Input;
		m_CursorPos = 0;
		if (m_Input)
		{
			m_PreviousCode = m_CurrentCode;
		}
		else
		{
			auto& assets = Assets::GetInstance();
			auto generator = assets.Get<StageGenerator>(AssetName::StageGenerator);
			auto stage = generator->GetLevelFromCode(m_CurrentCode);

			if (stage != std::nullopt)
			{
				assets.Get<Audio>(AssetName::SfxCodeOk)->Play();
			}
			else
			{
				m_CurrentCode = m_PreviousCode;
				assets.Get<Audio>(AssetName::SfxCodeWrong)->Play();
			}

		}
		return true;
	}

	bool StageCodeMenuItem::OnDirectionInput(MenuRoot& root, Direction direction)
	{
		if (m_Input)
		{
			switch (direction)
			{
			case bsf::Direction::Left:
				m_CursorPos = m_CursorPos > 0 ? m_CursorPos - 1 : m_CursorPos;
				break;
			case bsf::Direction::Right:
				m_CursorPos = m_CursorPos < StageCode::DigitCount - 1 ? m_CursorPos + 1 : m_CursorPos;
				break;
			case bsf::Direction::Up:
				m_CurrentCode[m_CursorPos] = m_CurrentCode[m_CursorPos] < 9 ? m_CurrentCode[m_CursorPos] + 1 : 0;
				break;
			case bsf::Direction::Down:
				m_CurrentCode[m_CursorPos] = m_CurrentCode[m_CursorPos] > 0 ? m_CurrentCode[m_CursorPos] - 1 : 9;
				break;
			default:
				break;
			}


			return true;
		}
		return false;
	}

	bool StageCodeMenuItem::OnKeyTyped(MenuRoot& root, int32_t keyCode)
	{
		if (m_Input && keyCode >= '0' && keyCode <= '9')
		{
			m_CurrentCode[m_CursorPos] = keyCode - '0';
			m_CursorPos = std::min(m_CursorPos + 1u, StageCode::DigitCount - 1u);
			return true;
		}

		return false;
	}

	void StageCodeMenuItem::Render(MenuRoot& root, Renderer2D& renderer)
	{
		static const std::string codeStrFormat = "%d%d%d%d-%d%d%d%d-%d%d%d%d";
		auto color = Selected ? s_SelectedMenuColor : s_MenuColor;
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		
		char codeStrBuffer[15];
		std::vector<glm::vec4> colors(StageCode::DigitCount + 2);
		auto& c = m_CurrentCode;

		std::sprintf(codeStrBuffer, codeStrFormat.c_str(), c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11]);
			
		std::fill(colors.begin(), colors.end(), color);

		if (m_Input)
			colors[m_CursorPos + m_CursorPos / 4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		
		renderer.Push();
		renderer.Scale({ 0.5f, 0.5f });
		renderer.Color(color);
		renderer.DrawString(font, "Stage Code");
		renderer.Translate({ 0.0f, -1.0f });
		renderer.Scale({ 2.0f, 2.0f });
		renderer.DrawString(font, codeStrBuffer, { 0.0f, 0.0f }, colors);
		renderer.Color({ 1.0f, 0.0f, 0.0f, 1.0f });
		renderer.Pop();
	}


	StageCodeMenuItem::StageCode::StageCode(uint64_t code)
	{
		uint64_t divider = std::pow(10, DigitCount - 1);
		for (uint8_t i = 0; i < DigitCount; i++)
		{
			m_Digits[i] = code / divider;
			assert(m_Digits[i] < 10);
			code %= divider;
			divider /= 10;
		}
	}

	uint8_t& StageCodeMenuItem::StageCode::operator[](uint8_t index)
	{
		assert(index < DigitCount);
		return m_Digits[index];
	}

	const uint8_t& StageCodeMenuItem::StageCode::operator[](uint8_t index) const
	{
		assert(index < DigitCount);
		return m_Digits[index];
	}

	StageCodeMenuItem::StageCode::operator uint64_t() const
	{
		uint64_t result = 0;
		for (uint8_t i = 0; i < DigitCount; i++)
			result += std::pow(10, DigitCount - 1 - i) * m_Digits[i];
		return result;
	}


}
