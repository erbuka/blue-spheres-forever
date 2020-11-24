#include "BsfPch.h"

#include "MenuScene.h"
#include "Renderer2D.h"
#include "Application.h"
#include "Assets.h"
#include "Font.h"
#include "Stage.h"
#include "GameScene.h"
#include "Audio.h"
#include "StageEditorScene.h"
#include "Texture.h"
#include "SkyGenerator.h"
#include "Framebuffer.h"
#include "ShaderProgram.h"
#include "VertexArray.h"

namespace bsf
{

	static constexpr float s_VirtualHeight = 10;
	static constexpr float s_SidePanelWidth = s_VirtualHeight / 2.0f;
	static constexpr float s_MenuPadding = s_VirtualHeight / 40.0f;

	static constexpr glm::vec4 s_SelectedMenuColor = Colors::Yellow;
	static constexpr glm::vec4 s_MenuColor = Colors::White;


	template<typename T>
	SelectMenuItem<T>::SelectMenuItem(const std::string& caption) :
		Caption(caption), m_SelectedOption(0)
	{
	}

	template<typename T>
	bool SelectMenuItem<T>::OnConfirm(MenuRoot& root)
	{
		m_SelectedOption = (m_SelectedOption + 1) % m_Options.size();
		return true;
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
		return true;
	}

	template<typename T>
	void SelectMenuItem<T>::Render(MenuRoot& root, Renderer2D& renderer)
	{
		auto color = Selected ? s_SelectedMenuColor : s_MenuColor;
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);


		renderer.Color(color);
		renderer.DrawStringShadow(font, m_Options[m_SelectedOption].first, Bounds.Position);
		renderer.Push();
		renderer.Translate(Bounds.Position + glm::vec2(0.0f, 1.0f));
		renderer.Scale({ 0.5f, 0.5f });
		renderer.DrawStringShadow(font, Caption);
		renderer.Pop();

	}

	template<typename T>
	void SelectMenuItem<T>::AddOption(const std::string& caption, const T& value)
	{
		m_Options.push_back({ caption, value });
	}

	template<typename T>
	const T& SelectMenuItem<T>::GetSelectedOption()
	{
		return m_Options[m_SelectedOption].second;
	}

	template<typename T>
	void SelectMenuItem<T>::SetSelectedOption(const T& val)
	{
		for (size_t i = 0; i < m_Options.size(); ++i)
		{
			if (m_Options[i].second == val)
			{
				m_SelectedOption = i;
				return;
			}
		}

		BSF_ERROR("Menu option not found");

	}

	void MenuScene::OnAttach()
	{
		// Textures
		m_txBackground = CreateCheckerBoard({ ToHexColor(Colors::DarkGray), ToHexColor(Colors::LightGray) });

		// Config
		m_Config = Config::Load();

		// Menus
		BuildMenus();
		
		// Fade In
		ScheduleTask(ESceneTaskEvent::PostRender, MakeRef<FadeTask>(glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 1.0f, 0.0f }, 0.5f));

	}

	void MenuScene::OnRender(const Time& time)
	{
		auto& assets = Assets::GetInstance();
		auto& r2 = GetApplication().GetRenderer2D();
		auto font = assets.Get<Font>(AssetName::FontMain);
		auto windowSize = GetApplication().GetWindowSize();

		float height = s_VirtualHeight;
		float width = windowSize.x / windowSize.y * s_VirtualHeight;
		float offset = glm::fract(time.Elapsed / 2.0f);
		
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		r2.Begin(glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f));
		r2.TextShadowColor({ 0.0f, 0.0f, 0.0f, 0.5f });
		r2.TextShadowOffset({ 0.025f, -0.025f });

		// Draw backgrund
		r2.Push();
		r2.Pivot(EPivot::BottomLeft);
		r2.Texture(m_txBackground);
		r2.DrawQuad({ 0.0f, 0.0f }, { width, height }, { width, height }, { offset, offset });
		r2.Pop();

		// Draw side panel
		r2.Push();
		r2.Pivot(EPivot::BottomLeft);
		r2.Color(Colors::Blue);
		r2.DrawQuad({ 0.0f, 0.0f }, { s_SidePanelWidth, height });
		r2.Pivot(EPivot::Center);
		for (size_t i = 0; i <= height + 1.0f; ++i)
		{
			r2.Push();
			r2.Translate({ s_SidePanelWidth, i - offset });
			r2.Rotate(glm::radians(45.0f));
			r2.DrawQuad();
			r2.Pop();
		}
		r2.Pop();


		// Draw Menus
		m_MenuRoot.ViewportSize = { width, height };
		m_MenuRoot.WindowSize = windowSize;
		m_MenuRoot.Render(r2);
		r2.End();
	}

	void MenuScene::OnDetach()
	{
		Assets::GetInstance().Get<Audio>(AssetName::SfxIntro)->FadeOut(0.5f);
	}
	
	void MenuScene::BuildMenus()
	{
		// Create menus
		auto mainMenu = MakeRef<Menu>();
		auto playMenu = MakeRef<Menu>();
		auto customStagesMenu = MakeRef<Menu>();
		auto optionsMenu = MakeRef<Menu>();

		auto backFn = [&](MenuRoot& root) { root.PopMenu(); return true; };

		playMenu->AddItem<ButtonMenuItem>("Play")->SetConfirmFunction([&](MenuRoot& root) {
			auto& stageGenerator = Assets::GetInstance().Get<StageGenerator>(AssetName::StageGenerator);
			auto code = m_StageCodeMenuItem->GetStageCode();
			auto stage = stageGenerator->Generate(code);
			PlayStage(stage, GameInfo{ GameMode::BlueSpheres, 0, stageGenerator->GetStageFromCode(code).value() });
			return true;
		});
		m_StageCodeMenuItem = playMenu->AddItem<StageCodeMenuItem>();
		playMenu->AddItem<ButtonMenuItem>("Back")->SetConfirmFunction(backFn);


		customStagesMenu->AddItem<ButtonMenuItem>("Play")->SetConfirmFunction([&](MenuRoot& root) {
			auto stageFile = m_SelectStageMenuItem->GetSelectedOption();
			auto stage = MakeRef<Stage>();
			stage->Load(stageFile);
			PlayStage(stage, GameInfo{ GameMode::CustomStage, 0, 0 });
			return true;
		});

		{ // Custom stage select
			m_SelectStageMenuItem = customStagesMenu->AddItem<SelectMenuItem<std::string>>("Stage");
			Stage stage;
			for (const auto& stageFile : Stage::GetStageFiles())
			{
				stage.Load(stageFile);
				m_SelectStageMenuItem->AddOption(stage.Name, stageFile);
			}
		}

		customStagesMenu->AddItem<ButtonMenuItem>("Stage Editor")->SetConfirmFunction([&](MenuRoot&) {
			auto task = ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(1.0f), 0.5f);
			task->SetDoneFunction([&](SceneTask&) {
				GetApplication().GotoScene(MakeRef<StageEditorScene>());
			});
			return true;
		});

		customStagesMenu->AddItem<ButtonMenuItem>("Back")->SetConfirmFunction(backFn);

		m_DisplayModeMenuItem = optionsMenu->AddItem<SelectMenuItem<DisplayModeDescriptor>>("Display Mode");
		m_FullscreenMenuItem = optionsMenu->AddItem<SelectMenuItem<bool>>("Fullscreen");
		optionsMenu->AddItem<ButtonMenuItem>("Back")->SetConfirmFunction([&](MenuRoot& root) {
			m_Config.DisplayMode = m_DisplayModeMenuItem->GetSelectedOption();
			m_Config.Fullscreen = m_FullscreenMenuItem->GetSelectedOption();
			m_Config.Save();
			GetApplication().LoadConfig();
			root.PopMenu();
			return true;
		});

		m_FullscreenMenuItem->AddOption("Yes", true);
		m_FullscreenMenuItem->AddOption("No", false);

		m_FullscreenMenuItem->SetSelectedOption(m_Config.Fullscreen);

		for (auto& mode : GetDisplayModes())
			m_DisplayModeMenuItem->AddOption(mode.ToString(), mode);

		m_DisplayModeMenuItem->SetSelectedOption(m_Config.DisplayMode);
		
			
		mainMenu->AddItem<LinkMenuItem>("Classic Mode", playMenu);
		mainMenu->AddItem<LinkMenuItem>("Custom Stages", customStagesMenu);
		mainMenu->AddItem<LinkMenuItem>("Options", optionsMenu);

		mainMenu->AddItem<ButtonMenuItem>("Exit")->SetConfirmFunction([&](MenuRoot&) {
			GetApplication().Exit();
			return true;
		});

		m_MenuRoot.PushMenu(mainMenu);

		// Events
		auto& app = GetApplication();
		AddSubscription(app.KeyPressed, [&](const KeyPressedEvent& evt) {
			switch (evt.KeyCode)
			{
			case GLFW_KEY_LEFT: m_MenuRoot.FireOnDirectionInput(Direction::Left); break;
			case GLFW_KEY_RIGHT: m_MenuRoot.FireOnDirectionInput(Direction::Right); break;
			case GLFW_KEY_UP: m_MenuRoot.FireOnDirectionInput(Direction::Up); break;
			case GLFW_KEY_DOWN: m_MenuRoot.FireOnDirectionInput(Direction::Down); break;
			case GLFW_KEY_ENTER: m_MenuRoot.FireOnConfirm(); break;
			}
		});

		AddSubscription(app.CharacterTyped, [&](const CharacterTypedEvent& evt) {
			m_MenuRoot.FireOnKeyTyped(evt.Character);
		});


		AddSubscription(app.MouseMoved, [&](const MouseEvent& evt) {
			m_MenuRoot.FireOnMouseMoved({ evt.X, evt.Y });
		});

		AddSubscription(app.MousePressed, [&](const MouseEvent& evt) {
			m_MenuRoot.FireOnMouseClicked({ evt.X, evt.Y });
		});
	}

	void MenuScene::PlayStage(const Ref<Stage>& stage, const GameInfo& gameInfo)
	{
		auto fadeTask = MakeRef<FadeTask>(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f);


		fadeTask->SetDoneFunction([&, stage, gameInfo](SceneTask& self) {
			GetApplication().GotoScene(MakeRef<GameScene>(stage, gameInfo));
		});

		ScheduleTask(ESceneTaskEvent::PostRender, fadeTask);
	}

	void MenuRoot::FireOnConfirm()
	{
		assert(m_MenuStack.size() > 0);
		if (m_MenuStack.top()->OnConfirm(*this))
		{
			Assets::GetInstance().Get<Audio>(AssetName::SfxMenu)->Play();
		}
	}

	void MenuRoot::FireOnDirectionInput(Direction direction)
	{
		assert(m_MenuStack.size() > 0);
		if (m_MenuStack.top()->OnDirectionInput(*this, direction))
		{
			Assets::GetInstance().Get<Audio>(AssetName::SfxMenu)->Play();
		}
	}

	void MenuRoot::FireOnKeyTyped(int32_t keyCode)
	{
		assert(m_MenuStack.size() > 0);

		if (m_MenuStack.top()->OnKeyTyped(*this, keyCode))
		{
			Assets::GetInstance().Get<Audio>(AssetName::SfxMenu)->Play();
		}
	}

	void MenuRoot::FireOnMouseMoved(const glm::vec2& pos)
	{
		assert(m_MenuStack.size() > 0);
		m_MenuStack.top()->OnMouseMoved(*this, WindowToViewport(pos));
	}

	void MenuRoot::FireOnMouseClicked(const glm::vec2& pos)
	{
		assert(m_MenuStack.size() > 0);
		m_MenuStack.top()->OnMouseClicked(*this, WindowToViewport(pos));
		BSF_INFO("Click");
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
		auto& currentMenu = m_MenuStack.top();
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);

		const float x = s_SidePanelWidth + 1.5f;
		float y = ViewportSize.y - s_MenuPadding;

		for (auto& child : currentMenu->GetChildren())
		{
			child->Bounds.Position = { x, y - child->GetHeight() };
			auto caption = child->GetCaption();
			child->Bounds.Size = { font->GetStringWidth(child->GetCaption()), child->GetHeight() };
			y -= child->GetHeight() + s_MenuPadding;
		}

		renderer.Pivot(EPivot::BottomLeft);
		m_MenuStack.top()->Render(*this, renderer);
	}

	Ref<Menu>& MenuRoot::GetCurrentMenu()
	{
		assert(m_MenuStack.size() > 0);
		return m_MenuStack.top();
	}

	glm::vec2 MenuRoot::WindowToViewport(const glm::vec2& pos) const
	{
		return {
			pos.x / WindowSize.x * ViewportSize.x,
			(1.0f - pos.y / WindowSize.y) * ViewportSize.y
		};
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

	bool Menu::OnMouseMoved(MenuRoot& root, const glm::vec2& pos)
	{
		bool handled = m_SelectedItem->OnMouseMoved(root, pos);

		if (!handled)
		{
			for (auto& child : m_Children)
			{
				if (child->Bounds.Contains(pos))
				{
					m_SelectedItem = child;
					handled = true;
					break;
				}
			}
		}

		return handled;
	}

	bool Menu::OnMouseClicked(MenuRoot& root, const glm::vec2& pos)
	{
		bool handled = m_SelectedItem->OnMouseClicked(root, pos);

		if (!handled)
		{
			if (m_SelectedItem->Bounds.Contains(pos))
			{
				m_SelectedItem->OnConfirm(root);
				handled = true;
			}
		}

		return handled;
	}

	void Menu::Render(MenuRoot& root, Renderer2D& renderer)
	{
		for (auto& child : m_Children)
		{
			child->Selected = m_SelectedItem == child;
			child->Render(root, renderer);
		}
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
		renderer.DrawStringShadow(font, Caption, Bounds.Position);
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
		renderer.DrawStringShadow(font, Caption, Bounds.Position);
	}


	
	StageCodeMenuItem::StageCodeMenuItem() : m_CursorPos(0)
	{
		auto generator = Assets::GetInstance().Get<StageGenerator>(AssetName::StageGenerator);
		m_CurrentCode = generator->GetCodeFromStage(1);
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
			auto stage = generator->GetStageFromCode(m_CurrentCode);

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
				m_CursorPos = m_CursorPos < StageCodeHelper::DigitCount - 1 ? m_CursorPos + 1 : m_CursorPos;
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
			m_CursorPos = std::min(m_CursorPos + 1u, StageCodeHelper::DigitCount - 1u);
			return true;
		}

		return false;
	}

	bool StageCodeMenuItem::OnMouseMoved(MenuRoot& root, const glm::vec2& pos)
	{
		return m_Input;
	}

	void StageCodeMenuItem::Render(MenuRoot& root, Renderer2D& renderer)
	{
		auto color = Selected ? s_SelectedMenuColor : s_MenuColor;
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		
		auto& code = m_CurrentCode;

		FormattedString codeString;
		for (uint32_t i = 0; i < code.DigitCount; i++)
		{

			if (i > 0 && i % 4 == 0)
			{
				codeString.Color(color);
				codeString += "-";
			}

			codeString.Color(m_Input && m_CursorPos == i ? glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f } : color);
			codeString += std::to_string(code[i]);

		}

		renderer.Color(color);
		renderer.DrawStringShadow(font, codeString, Bounds.Position);
		renderer.Push();
		renderer.Translate(Bounds.Position + glm::vec2(0.0f, 1.0f));
		renderer.Scale(0.5f);
		renderer.DrawStringShadow(font, "Stage Code");
		renderer.Pop();

	}





}
