#include "BsfPch.h"

#include "StageEditorScene.h"
#include "MenuScene.h"
#include "Assets.h"
#include "Texture.h"
#include "Renderer2D.h"
#include "Application.h"
#include "Font.h"
#include "Common.h"


namespace bsf
{
	static constexpr float s_uiScale = 20.0f;
	static constexpr float s_uiTopBarHeight = s_uiScale / 10.0f;
	static constexpr float s_uiPropertiesWidth = s_uiScale / 2.0f;
	static constexpr float s_uiToolbarMargin = s_uiScale / 100.0f;
	static constexpr float s_uiToolbarButtonSize = s_uiTopBarHeight - 2 * s_uiToolbarMargin;
	static constexpr float s_uiStageListItemWidth = s_uiScale / 3.0f;
	static constexpr float s_uiStageListItemHeight = s_uiScale / 5.0f;
	static constexpr float s_uiConfirmDialogBtnSize = s_uiScale / 6.0f;

	static constexpr uint32_t s_MinStageSize = 32;
	static constexpr uint32_t s_MaxStageSize = 256;

	static constexpr uint32_t s_MinStageSizePower = Log2(s_MinStageSize);
	static constexpr uint32_t s_MaxStageSizePower = Log2(s_MaxStageSize);


	void StageEditorScene::OnAttach()
	{
		m_CurrentStage = MakeRef<Stage>();
		InitializeUI();
		ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, glm::vec4(1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), 0.5f);
	}
	
	void StageEditorScene::OnRender(const Time& time)
	{
		auto& app = GetApplication();
		auto& r2 = app.GetRenderer2D();

		const auto windowSize = app.GetWindowSize();

		const auto height = s_uiScale;
		const auto width = windowSize.x / windowSize.y * height;

		GLEnableScope scope({ GL_DEPTH_TEST });

		glViewport(0, 0, windowSize.x, windowSize.y);
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		m_uiRoot->Render(windowSize, { width, height }, r2, time);


	}

	void StageEditorScene::OnDetach()
	{
		m_uiRoot->Detach(GetApplication());
		m_uiConfirmDialogLayer->Traverse([](auto& el) { el.ClearSubscriptions(); });
		m_uiStageListDialogLayer->Traverse([](auto& el) {el.ClearSubscriptions(); });
	}

	void StageEditorScene::ShowConfirmDialog(const std::string& text, const std::function<void()>& onConfirm)
	{
		m_uiConfirmDialogText->Text = text;
		m_uiConfirmDialogPanel->Pack();
		m_OnConfirmDialogConfirm = onConfirm;
		m_uiRoot->PushLayer(m_uiConfirmDialogLayer);
	}

	void StageEditorScene::OnSaveButtonClick(const MouseEvent& evt)
	{
		Trim(m_CurrentStage->Name);

		if (m_CurrentStageFile.has_value())
		{
			m_CurrentStage->Save(m_CurrentStageFile.value());
		}
		else
		{
			std::stringstream fileName;
			fileName << "assets/data/" << std::hex << UniqueId() << ".bssj";
			m_CurrentStageFile = fileName.str();
			m_CurrentStage->Save(m_CurrentStageFile.value());
		}

		m_uiRoot->ShowToast({ FormattedString("Stage Saved: ").Color(Colors::Yellow).Add(m_CurrentStage->Name) });
	}

	void StageEditorScene::OnStageListButtonClick(const MouseEvent& evt)
	{
		auto files = Stage::GetStageFiles();
		m_uiStageList->SetFiles(std::move(files));
		m_uiRoot->PushLayer(m_uiStageListDialogLayer);
	}

	void StageEditorScene::NewStage()
	{
		m_CurrentStageFile = std::nullopt;
		(*m_CurrentStage) = std::move(Stage(32));
	}

	void StageEditorScene::LoadStage(std::string_view fileName)
	{
		if(m_CurrentStage->Load(fileName))
			m_CurrentStageFile = std::string(fileName.data());
	}

	void StageEditorScene::InitializeUI()
	{
		auto& assets = Assets::GetInstance();

		UIStyle style;
		style.GlobalScale = s_uiScale;

		m_uiRoot = MakeRef<UIRoot>();
		m_uiRoot->Attach(GetApplication());
		m_uiRoot->SetStyle(style);

		const auto makeToolBarButton = [&](AssetName icon, const glm::vec4& tint) {
			auto btn = MakeRef<UIIconButton>();
			btn->PreferredSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
			btn->MaxSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
			btn->Icon = assets.Get<Texture2D>(icon);
			btn->Tint = tint;
			return btn;
		};
		
		// Editor layer
		{
			// Top Bar
			auto topBar = MakeRef<UIPanel>();
			topBar->Layout = UILayout::Horizontal;
			topBar->PreferredSize = { 0.0f, s_uiTopBarHeight };

			// Back button
			{
				auto btn = makeToolBarButton(AssetName::TexUIBack, Colors::White);
				AddSubscription(btn->MouseClicked, [&](const MouseEvent&) {
					auto task = ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(1.0f), 0.5f);
					task->SetDoneFunction([&](SceneTask& self) {
						GetApplication().GotoScene(MakeRef<MenuScene>());
					});
				});
				topBar->AddChild(btn);
			}

			auto title = MakeRef<UIText>();
			title->Text = "Stage Editor";
			topBar->AddChild(title);


			// New button
			{
				auto btn = makeToolBarButton(AssetName::TexUINew, Colors::White);
				AddSubscription(btn->MouseClicked, this, &StageEditorScene::OnNewButtonClick);
				topBar->AddChild(btn);
			}



			// Stage list button
			{
				auto btn = makeToolBarButton(AssetName::TexUIOpen, Colors::White);
				AddSubscription(btn->MouseClicked, this, &StageEditorScene::OnStageListButtonClick);
				topBar->AddChild(btn);
			}
			
			// Save button
			{
				auto btn = makeToolBarButton(AssetName::TexUISave, Colors::White);
				AddSubscription(btn->MouseClicked, this, &StageEditorScene::OnSaveButtonClick);
				topBar->AddChild(btn);
			}

			// Delete button
			{
				auto btn = makeToolBarButton(AssetName::TexUIDelete, Colors::White);
				AddSubscription(btn->MouseClicked, [&](const MouseEvent&) {
					if (m_CurrentStageFile.has_value())
					{
						ShowConfirmDialog("Do you really want to delete this stage?", [&] {
							std::filesystem::remove(m_CurrentStageFile.value());
							NewStage();
							m_uiRoot->PopLayer();
						});
					}
				});
				topBar->AddChild(btn);
			}



			auto tools = {
				std::make_tuple(StageEditorTool::BlueSphere,	AssetName::TexUISphere,			Colors::BlueSphere),
				std::make_tuple(StageEditorTool::RedSphere,		AssetName::TexUISphere,			Colors::RedSphere),
				std::make_tuple(StageEditorTool::YellowSphere,	AssetName::TexUISphere,			Colors::YellowSphere),
				std::make_tuple(StageEditorTool::GreenSphere,	AssetName::TexUISphere,			Colors::GreenSphere),
				std::make_tuple(StageEditorTool::Bumper,		AssetName::TexUISphere,			Colors::White),
				std::make_tuple(StageEditorTool::Ring,			AssetName::TexUIRing,			Colors::Ring),
				std::make_tuple(StageEditorTool::AvoidSearch,	AssetName::TexUIAvoidSearch,	Colors::White),
				std::make_tuple(StageEditorTool::Position,		AssetName::TexUIPosition,		Colors::White),
			};

			for (const auto& toolSpec : tools)
			{

				auto btn = makeToolBarButton(std::get<1>(toolSpec), std::get<2>(toolSpec));

				AddSubscription(btn->MouseClicked, [&, btn, tool = std::get<0>(toolSpec)](const MouseEvent& evt) {
					for (auto& b : m_ToolButtons)
						b->Selected = false;
					m_uiEditorArea->ActiveTool = tool;
					btn->Selected = true;
				});

				m_ToolButtons.push_back(btn);
				topBar->AddChild(btn);
			}

		


			// Properties
			auto properties = MakeRef<UIPanel>();
			properties->Layout = UILayout::Vertical;
			properties->PreferredSize = { s_uiPropertiesWidth, 0.0f };

			// Stage name
			{
				UIBoundValue<std::string> val;
				val.Get = [&] { return m_CurrentStage->Name; };
				val.Set = [&](const std::string& val) { m_CurrentStage->Name = val; };
				auto stageNameInput = MakeRef<UITextInput>();
				stageNameInput->Label = "Stage Name";
				stageNameInput->Bind(val);
				properties->AddChild(stageNameInput);
			}

			{ // Max rings + stage size
				auto panel = MakeRef<UIPanel>();
				panel->Layout = UILayout::Horizontal;

				// Max Rings input
				{
					UIBoundValue<std::string> val;

					val.Set = [&, check = std::regex("^[0-9]+$")](const std::string& val) {
						if (std::regex_match(val, check))
						{
							m_CurrentStage->MaxRings = std::atoi(val.c_str());
						}
						else if (val.empty())
						{
							m_CurrentStage->MaxRings = 0;
						}
					};

					val.Get = [&] { return std::to_string(m_CurrentStage->MaxRings); };


					auto maxRingsInput = MakeRef<UITextInput>();
					maxRingsInput->Label = "Max Rings";
					maxRingsInput->Bind(val);
					maxRingsInput->PreferredSize.x = s_uiPropertiesWidth / 2.0f;
					panel->AddChild(maxRingsInput);
				}


				{ // Stage size
					auto stageSizePanel = MakeRef<UIPanel>();
					stageSizePanel->Layout = UILayout::Vertical;
					stageSizePanel->Margin = 1.0f;
					stageSizePanel->HasShadow = true;
					stageSizePanel->BackgroundColor = style.Palette.BackgroundVariant;
					stageSizePanel->PreferredSize.x = s_uiPropertiesWidth / 2.0f;

					auto stageSizeLabel = MakeRef<UIText>();
					stageSizeLabel->Text = "Size";
					stageSizeLabel->Scale = style.LabelFontScale;
					stageSizePanel->AddChild(stageSizeLabel);

					UIBoundValue<float> sizeValue(0.0f);


					sizeValue.Get = [&] { return (float)m_CurrentStage->GetSize(); };
					sizeValue.Set = [&](const float& v) { m_CurrentStage->Resize((int32_t)v); };
					sizeValue.Format = [&] {return std::to_string(m_CurrentStage->GetSize()); };

					auto stageSizeSlider = MakeRef<UISlider>();
					stageSizeSlider->Min = s_MinStageSize;
					stageSizeSlider->Max = s_MaxStageSize;
					stageSizeSlider->ForegroundColor = style.Palette.Background;
					stageSizeSlider->ShowValue = true;
					stageSizeSlider->Bind(sizeValue);

					stageSizePanel->AddChild(stageSizeSlider);

					panel->AddChild(stageSizePanel);
				}

				properties->AddChild(panel);

			}


			// Pattern Colors
			{

				auto panel = MakeRef<UIPanel>();
				panel->Layout = UILayout::Horizontal;

				auto cp1 = MakeRef<UIColorPicker>();
				cp1->PreferredSize.x = s_uiPropertiesWidth / 2.0f;
				cp1->Bind([&]() -> glm::vec3& { return m_CurrentStage->PatternColors[0]; });
				cp1->Label = "Primary";
				panel->AddChild(cp1);

				auto cp2 = MakeRef<UIColorPicker>();
				cp2->PreferredSize.x = s_uiPropertiesWidth / 2.0f;
				cp2->Bind([&]() -> glm::vec3& { return m_CurrentStage->PatternColors[1]; });
				cp2->Label = "Secondary";

				panel->AddChild(cp2);

				properties->AddChild(panel);

			}

			// Sky and stars
			{

				auto panel = MakeRef<UIPanel>();
				panel->Layout = UILayout::Horizontal;

				auto cp1 = MakeRef<UIColorPicker>();
				cp1->PreferredSize.x = s_uiPropertiesWidth / 2.0f;
				cp1->Bind([&]() -> glm::vec3& { return m_CurrentStage->SkyColor; });
				cp1->Label = "Sky";
				panel->AddChild(cp1);

				auto cp2 = MakeRef<UIColorPicker>();
				cp2->PreferredSize.x = s_uiPropertiesWidth / 2.0f;
				cp2->Bind([&]() -> glm::vec3& { return m_CurrentStage->StarsColor; });
				cp2->Label = "Stars";

				panel->AddChild(cp2);

				properties->AddChild(panel);

			}
			// Emerald color
			{
				auto cp1 = MakeRef<UIColorPicker>();
				cp1->Bind([&]() -> glm::vec3& { return m_CurrentStage->EmeraldColor; });
				cp1->Label = "Emerald";
				properties->AddChild(cp1);
			}


			// Editor area
			m_uiEditorArea = MakeRef<UIStageEditorArea>();
			m_uiEditorArea->PreferredSize = { -s_uiPropertiesWidth, 0.0f };
			m_uiEditorArea->SetStage(m_CurrentStage);

			// Middle section
			auto middle = MakeRef<UIPanel>();
			middle->Layout = UILayout::Horizontal;
			middle->PreferredSize = { 0.0f, -s_uiTopBarHeight };

			middle->AddChild(m_uiEditorArea);
			middle->AddChild(properties);

			// Main panel
			auto main = MakeRef<UILayer>();
			main->Layout = UILayout::Vertical;

			main->AddChild(topBar);
			main->AddChild(middle);

			m_uiRoot->PushLayer(main);
		}

		// Stage list dialog layer

		{
			m_uiStageListDialogLayer = MakeRef<UILayer>();
			m_uiStageListDialogLayer->Layout = UILayout::Fill;
			m_uiStageListDialogLayer->BackgroundColor = style.ShadowColor;
			AddSubscription(m_uiStageListDialogLayer->MouseClicked, [&](auto&) { m_uiRoot->PopLayer(); });

			auto dialogPanel = MakeRef<UIPanel>();
			dialogPanel->Layout = UILayout::Horizontal;
			dialogPanel->HasShadow = true;
			dialogPanel->Pack();

			// Stage list
			m_uiStageList = MakeRef<UIStageList>(4, 4);
			m_uiStageList->ItemWidth = s_uiStageListItemWidth;
			m_uiStageList->ItemHeight = s_uiStageListItemHeight;
			
			AddSubscription(m_uiStageList->FileSelected, [&](const UIStageList::StageSelectedEvent& evt) { 
				LoadStage(evt.FileName); 
				m_uiRoot->PopLayer();
			});
			
			AddSubscription(m_uiStageList->FileReorder, [&](const UIStageList::StageReorderEvent& evt) {
				Stage::SaveStageFiles(evt.Files);
			});
			
			dialogPanel->AddChild(m_uiStageList);

			// Scroll slider
			{
				UIBoundValue<float> val = 0.0f;
				val.Get = [&] {return m_uiStageList->GetScroll(); };
				val.Set = [&](const float& v) { m_uiStageList->SetScroll(v); };

				auto scrollSlider = MakeRef<UISlider>();
				scrollSlider->Orientation = UISliderOrientation::Vertical;
				scrollSlider->ForegroundColor = style.Palette.BackgroundVariant;
				scrollSlider->Bind(val);

				dialogPanel->AddChild(scrollSlider);
			}

			m_uiStageListDialogLayer->AddChild(dialogPanel);

		}

		// Confirm dialog layer
		{
			m_uiConfirmDialogLayer = MakeRef<UILayer>();
			m_uiConfirmDialogLayer->Layout = UILayout::Fill;
			m_uiConfirmDialogLayer->BackgroundColor = style.ShadowColor;
			AddSubscription(m_uiConfirmDialogLayer->MouseClicked, [&](const MouseEvent&) { m_uiRoot->PopLayer(); });

			m_uiConfirmDialogPanel = MakeRef<UIPanel>();
			m_uiConfirmDialogPanel->Layout = UILayout::Vertical;
			m_uiConfirmDialogPanel->HasShadow = true;
			m_uiConfirmDialogPanel->Margin = 2.0f;
			m_uiConfirmDialogLayer->AddChild(m_uiConfirmDialogPanel);
			
			m_uiConfirmDialogText = MakeRef<UIText>();
			m_uiConfirmDialogPanel->AddChild(m_uiConfirmDialogText);

			auto buttonsPanel = MakeRef<UIPanel>();
			buttonsPanel->Layout = UILayout::Horizontal;


			{
				auto btn = MakeRef<UIButton>();
				btn->Label = "No";
				btn->PreferredSize.x = s_uiConfirmDialogBtnSize;
				AddSubscription(btn->MouseClicked, [&](const MouseEvent&) { m_uiRoot->PopLayer(); });
				buttonsPanel->AddChild(btn);
			}

			{
				auto btn = MakeRef<UIButton>();
				btn->Label = "Yes";
				btn->PreferredSize.x = s_uiConfirmDialogBtnSize;
				AddSubscription(btn->MouseClicked, [&](const MouseEvent&) { m_OnConfirmDialogConfirm(); });
				buttonsPanel->AddChild(btn);
			}

			m_uiConfirmDialogPanel->AddChild(buttonsPanel);



		}

	}


	#pragma region UI Impl


	void UIPanel::AddChild(const Ref<UIElement>& child)
	{
		m_Children.push_back(child);
	}




	void UIPanel::Update(const UIRoot& root, const Time& time)
	{

		const float margin = GetStyle().GetMargin(Margin);

		glm::vec2 minSizeChildren = { 0, 0 };
		glm::vec2 maxMinChildSize = { 0, 0 };

		for (auto& child : m_Children)
		{
			child->Update(root, time);
			minSizeChildren += child->MinSize;
			maxMinChildSize = glm::max(maxMinChildSize, child->MinSize);
		}

		switch (Layout)
		{
		case UILayout::Horizontal:
			MinSize.x = minSizeChildren.x + 2.0f * margin;
			MinSize.y = maxMinChildSize.y + 2.0f * margin;
			break;
		case UILayout::Vertical:
			MinSize.x = maxMinChildSize.x + 2.0f * margin;
			MinSize.y = minSizeChildren.y + 2.0f * margin;
			break;
		}

		if (m_ShouldPack)
		{
			//MaxSize = glm::max(PreferredSize, MinSize);
			MaxSize = glm::max(PreferredSize, MinSize);
			m_ShouldPack = false;
		}

		
	}

	void UIPanel::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& style = GetStyle();
		float margin = style.GetMargin(Margin);
		auto contentBounds = Bounds;
		contentBounds.Shrink(margin);

		if (HasShadow)
		{
			r2.Push();
			r2.Translate({ style.ShadowOffset, -style.ShadowOffset });
			r2.Color(style.ShadowColor);
			r2.DrawQuad(contentBounds.Position, contentBounds.Size);
			r2.Pop();
		}

		r2.Push();
		r2.Color(style.GetBackgroundColor(*this, style.Palette.Background));
		r2.DrawQuad(contentBounds.Position, contentBounds.Size);
		r2.Pop();

		for (auto& child : m_Children)
			child->Render(root, r2, time);

	}

	void UIPanel::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
		auto& style = GetStyle();
		const float margin = style.GetMargin(Margin);
		auto contentBounds = Bounds;
		contentBounds.Shrink(margin);

		if (m_Children.size() > 0)
		{

			if (Layout == UILayout::Vertical)
			{
				glm::vec2 origin = contentBounds.Position + glm::vec2(0.0f, contentBounds.Height());

				// Resize children
				for (auto& child : m_Children)
				{
					glm::vec2 computedSize = {
						std::clamp(contentBounds.Width(), child->MinSize.x, child->MaxSize.x),
						std::clamp(child->PreferredSize.y >= 0 ? child->PreferredSize.y : contentBounds.Height() + child->PreferredSize.y, child->MinSize.y, child->MaxSize.y)
					};
					
					origin.y -= computedSize.y;

					child->UpdateBounds(root, origin, computedSize);


				}
			}
			else if (Layout == UILayout::Horizontal)
			{
				glm::vec2 origin = contentBounds.Position;

				for (auto& child : m_Children)
				{
					glm::vec2 computedSize = {
						std::clamp(child->PreferredSize.x >= 0 ? child->PreferredSize.x : contentBounds.Width() + child->PreferredSize.x, child->MinSize.x, child->MaxSize.x),
						std::clamp(contentBounds.Height(), child->MinSize.y, child->MaxSize.y)
					};

					child->UpdateBounds(root, origin + glm::vec2(0.0f, (contentBounds.Height() - computedSize.y) / 2.0f), computedSize);
					origin.x += computedSize.x;
				}
			}
			else if (Layout == UILayout::Fill)
			{
				if (m_Children.size() > 1)
					BSF_WARN("More than 1 child in 'Fill' layout");

				glm::vec2 origin = contentBounds.Position;

				auto& child = m_Children.front();
				
				glm::vec2 computedSize = glm::vec2(std::clamp(contentBounds.Width(), child->MinSize.x, child->MaxSize.x),
					std::clamp(contentBounds.Height(), child->MinSize.y, child->MaxSize.y));

				child->UpdateBounds(root, 
					origin + glm::vec2((contentBounds.Width() - computedSize.x) / 2.0f, (contentBounds.Height() - computedSize.y) / 2.0f), computedSize);
			}
			else
			{

				glm::vec2 origin = contentBounds.Position;


				for (auto& child : m_Children)
				{
					child->UpdateBounds(root, origin + child->Position, glm::clamp(child->PreferredSize, child->MinSize, child->MaxSize));
				}
			}
		}


	}

	uint32_t UIElement::m_NextId = 1;

	UIElement::UIElement() : UIElement(MakeFlags(UIElementFlags::ReceiveHover))
	{
	}

	UIElement::UIElement(std::underlying_type_t<UIElementFlags> flags) :
		m_Flags(flags),
		m_Id(m_NextId++)
	{
	}

#ifdef BSF_ENABLE_DIAGNOSTIC
	void UIElement::RenderDebugInfo(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		if (Hovered)
		{
			const std::array<std::pair<std::string, std::string>, 3> infos = {
				std::pair<std::string, std::string>{ "Position", fmt::format("{0:.2f} {1:.2f}", Bounds.Left(), Bounds.Bottom()) },
				std::pair<std::string, std::string>{ "Size", fmt::format("{0:.2f} {1:.2f}", Bounds.Width(), Bounds.Height()) },
				std::pair<std::string, std::string>{ "Margin", fmt::format("{0:.2f}", Margin) }
			};
			std::array<std::string, infos.size()> infoStrings;
			auto& font = Assets::GetInstance().Get<Font>(AssetName::FontText);
			float width = 0.0f;

			for (size_t i = 0; i < infos.size(); ++i)
			{
				infoStrings[i] = infos[i].first + ": " + infos[i].second;
				width = std::max(width, font->GetStringWidth(infoStrings[i]));
			}

			r2.Push();
			r2.Pivot(EPivot::TopLeft);
			r2.Translate({ Bounds.Left(), Bounds.Top() });
			r2.Color({ 0.0f, 0.0f, 0.0f, 0.75f });
			r2.Scale(0.25f); 
			r2.DrawQuad({ 0, 0 }, { width, infoStrings.size() });
			r2.Color(Colors::Yellow);
			for (const auto& str : infoStrings)
			{
				r2.DrawString(font, str);
				r2.Translate({ 0.0f, -1.0f });
			}
			r2.Pop();
		}
	}
#endif

	bool UIElement::GetFlag(UIElementFlags flag) const
	{
		return m_Flags && static_cast<std::underlying_type_t<UIElementFlags>>(flag);
	}

	const std::vector<Ref<UIElement>>& UIElement::Children()
	{
		static const std::vector<Ref<UIElement>> s_Empty;
		return s_Empty;
	}

	void UIElement::Traverse(const std::function<void(UIElement&)>& action)
	{
		for (auto& child : Children())
		{
			child->Traverse(action);
		}
		action(*this);
	}


	void UIRoot::SetStyle(const UIStyle& style)
	{
		m_Style = style;
		m_Style.Recompute();
	}

	void UIRoot::Attach(Application& app)
	{
		m_App = &app;

		AddSubscription(app.CharacterTyped, [&](const CharacterTypedEvent& evt) {
			if (m_FocusedControl)
				m_FocusedControl->CharacterTyped.Emit(evt);
		});

		AddSubscription(app.KeyPressed, [&](const KeyPressedEvent& evt) {
			if (m_FocusedControl)
				m_FocusedControl->KeyPressed.Emit(evt);
		});

		AddSubscription(app.KeyReleased, [&](const KeyReleasedEvent& evt) {
			if (m_FocusedControl)
				m_FocusedControl->KeyReleased.Emit(evt);
		});

		AddSubscription(app.MouseMoved, [&](const MouseEvent& evt) {
			if (m_Layers.empty())
				return;

			glm::vec2 pos = GetMousePosition({ evt.X,evt.Y });

			m_MouseState.PrevPosition = m_MouseState.Position;
			m_MouseState.Position = pos;
			m_MouseState.HoverTarget = nullptr;

			m_Layers.back()->Traverse([](UIElement& element) { element.Hovered = false; });


			if (m_MouseState.DragTarget)
			{
				auto delta = m_MouseState.Position - m_MouseState.PrevPosition;
				m_MouseState.DragTarget->MouseDragged.Emit({ pos.x, pos.y, delta.x, delta.y, m_MouseState.DragButton });
			}

			auto intersection = Intersect(m_Layers.back(), pos);
			if (intersection)
			{

				intersection->MouseMoved.Emit({ pos.x, pos.y, 0, 0, MouseButton::None });
				
				m_MouseState.HoverTarget = intersection;

				if (intersection->GetFlag(UIElementFlags::ReceiveHover))
					intersection->Hovered = true;

			}

		});

		AddSubscription(app.Wheel, [&](const WheelEvent& evt) {
			if (m_MouseState.HoverTarget)
			{
				auto pos = GetMousePosition({ evt.X, evt.Y });
				m_MouseState.HoverTarget->Wheel.Emit({ evt.DeltaX, evt.DeltaY, pos.x, pos.y });
			}
		});

		AddSubscription(app.MousePressed, [&](const MouseEvent& evt) {
			if (m_Layers.empty())
				return;

			glm::vec2 pos = GetMousePosition({ evt.X,evt.Y });

			m_MouseState.Position = pos;
			m_MouseState.PrevPosition = pos;

			auto& buttonState = m_MouseButtonState[evt.Button];
			buttonState.Pressed = true;
			buttonState.TimePressed = std::chrono::system_clock::now();

			auto intersection = Intersect(m_Layers.back(), pos);

			m_Layers.back()->Traverse([](UIElement& el) {el.Focused = false; });
			m_FocusedControl = nullptr;

			if (intersection)
			{
				intersection->MousePressed.Emit({ pos.x, pos.y, 0, 0, evt.Button });

				m_MouseState.DragTarget = intersection;
				m_MouseState.DragButton = evt.Button;

				if (intersection->GetFlag(UIElementFlags::ReceiveFocus))
				{
					intersection->Focused = true;
					m_FocusedControl = intersection;
				}

			}
			else
			{
				m_MouseState.DragTarget = nullptr;
			}
		});

		AddSubscription(app.MouseReleased, [&](const MouseEvent& evt) {

			if (m_Layers.empty())
				return;

			glm::vec2 pos = GetMousePosition({ evt.X,evt.Y });

			m_MouseState.DragTarget = nullptr;
			m_MouseState.Position = pos;
			m_MouseState.PrevPosition = pos;

			auto intersection = Intersect(m_Layers.back(), pos);
			auto& buttonState = m_MouseButtonState[evt.Button];
			buttonState.Pressed = false;

			m_Layers.back()->Traverse([&](UIElement& el) { el.GlobalMouseReleased.Emit({ pos.x, pos.y, 0, 0, evt.Button }); });

			if (intersection)
			{
				intersection->MouseReleased.Emit({ pos.x, pos.y, 0, 0, evt.Button });

				const auto now = std::chrono::system_clock::now();

				if (now - buttonState.TimePressed < s_ClickDelay)
				{
					intersection->MouseClicked.Emit({ pos.x, pos.y, 0, 0, evt.Button });

					if (now - buttonState.TimeLastClicked < s_ClickDelay)
						intersection->MouseDblClicked.Emit({ pos.x, pos.y, 0, 0, evt.Button });

					buttonState.TimeLastClicked = now;

				}
			}
		});
	}

	void UIRoot::Detach(Application& app)
	{
		for (auto& layer : m_Layers)
			layer->Traverse([](UIElement& el) { el.ClearSubscriptions(); });
		ClearSubscriptions();
	}

	void UIRoot::Render(const glm::vec2& windowSize, const glm::vec2& viewport, Renderer2D& r2, const Time& time)
	{
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);

		m_WindowSize = windowSize;
		m_Viewport = viewport;
		m_Projection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y, -1.0f, 1.0f);
		m_InverseProjection = glm::inverse(m_Projection);

		for (uint32_t i = 0; i < m_LayersToPop; ++i)
			m_Layers.pop_back();
		m_LayersToPop = 0;

		std::copy(m_LayersToPush.begin(), m_LayersToPush.end(), std::back_inserter(m_Layers));
		m_LayersToPush.clear();

		m_Style.Recompute();

		r2.Begin(m_Projection);

		r2.TextShadowColor(m_Style.ShadowColor);
		r2.TextShadowOffset({ m_Style.TextShadowOffset, -m_Style.TextShadowOffset });
		
		for (auto& layer : m_Layers)
		{
			layer->Traverse([&](UIElement& el) { 
				el.m_Style = &m_Style;
				el.m_App = m_App;
			});
			layer->Update(*this, time);
			layer->UpdateBounds(*this, { 0.0f, 0.0f }, viewport);
			layer->Render(*this, r2, time);
#ifdef BSF_ENABLE_DIAGNOSTIC
			//layer->Traverse([&](UIElement& el) { el.RenderDebugInfo(*this, r2, time); });
#endif
		}

		r2.Push();
		r2.Pivot(EPivot::Left);
		r2.Translate({ (viewport.x - m_Style.ToastWidth) / 2.0f, m_Style.GetMargin(2.0f) + 0.5f });

		for (auto& toast : m_Toasts)
		{
			r2.Color(m_Style.ShadowColor);
			r2.DrawQuad({ m_Style.ShadowOffset, -m_Style.ShadowOffset }, { m_Style.ToastWidth, 1.0f });

			r2.Color(m_Style.Palette.BackgroundVariant);
			r2.DrawQuad({ 0.0f, 0.0f }, { m_Style.ToastWidth, 1.0f });

			r2.Push();
			r2.Scale(0.5f);
			r2.Color(m_Style.Palette.Foreground);
			r2.DrawStringShadow(font, toast.Message, { m_Style.GetMargin(1.0f), 0.0f });
			r2.Pop();

			r2.Translate({ 0.0f, 1.0f + m_Style.GetMargin(2.0f) });


			toast.Duration = std::max(0.0f, toast.Duration - time.Delta);

		}

		m_Toasts.remove_if([](const auto& toast) -> bool { return toast.Duration == 0.0f; });

		r2.Pop();

		r2.End();
	}

	glm::vec2 UIRoot::GetMousePosition(const glm::vec2& windowMousePos) const
	{
		glm::vec4 ndc = { windowMousePos.x / m_WindowSize.x * 2.0 - 1.0f, 1.0f - windowMousePos.y / m_WindowSize.y * 2.0f, 0.0f, 1.0f };
		return m_InverseProjection * ndc;
	}

	Ref<UIElement> UIRoot::Intersect(const Ref<UIElement>& root, const glm::vec2& pos, const std::function<bool(const UIElement&)>& predicate)
	{
		static auto defaultPredicate = [](const auto&) -> bool {return true; };

		for (const auto& child : root->Children())
		{
			auto result = Intersect(child, pos, predicate);
			if (result != nullptr)
				return result;
		}

		return root->Bounds.Contains(pos) && (predicate ? predicate : defaultPredicate)(*root) ? root : nullptr;
	}



	void UIIconButton::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UIIconButton::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& style = GetStyle();

		r2.Push();
		r2.Texture(Icon);

		r2.Push();
		{
			r2.Translate({ style.ShadowOffset, -style.ShadowOffset });
			r2.Color(style.ShadowColor);
			r2.DrawQuad(Bounds.Position, Bounds.Size);
		}
		r2.Pop();

		r2.Color((Hovered || Selected) ? Tint : style.IconButtonDefaultTint * Tint);
		r2.DrawQuad(Bounds.Position, Bounds.Size);
		r2.Pop();
	}


	UIStageEditorArea::UIStageEditorArea() : UIElement(MakeFlags(UIElementFlags::ReceiveHover))
	{
		auto& assets = Assets::GetInstance();

		static const std::unordered_map<StageEditorTool, EStageObject> s_toolMap = {
			{ StageEditorTool::BlueSphere,		EStageObject::BlueSphere },
			{ StageEditorTool::RedSphere,		EStageObject::RedSphere },
			{ StageEditorTool::YellowSphere,	EStageObject::YellowSphere },
			{ StageEditorTool::Bumper,			EStageObject::Bumper },
			{ StageEditorTool::Ring,			EStageObject::Ring },
			{ StageEditorTool::GreenSphere,		EStageObject::GreenSphere}
		};

		m_StageObjRendering = {
			{ EStageObject::BlueSphere,		{ assets.Get<Texture2D>(AssetName::TexUISphere),	Colors::BlueSphere }},
			{ EStageObject::RedSphere,		{ assets.Get<Texture2D>(AssetName::TexUISphere),	Colors::RedSphere }},
			{ EStageObject::GreenSphere,	{ assets.Get<Texture2D>(AssetName::TexUISphere),	Colors::GreenSphere }},
			{ EStageObject::YellowSphere,	{ assets.Get<Texture2D>(AssetName::TexUISphere),	Colors::YellowSphere }},
			{ EStageObject::Bumper,			{ assets.Get<Texture2D>(AssetName::TexUISphere),	Colors::White }},
			{ EStageObject::Ring,			{ assets.Get<Texture2D>(AssetName::TexUIRing),		Colors::Ring }}
		};

		m_Pattern = CreateCheckerBoard({ 0, 0 });
		m_BgPattern = CreateCheckerBoard({ 0xffffffff, 0xffcccccc });

		m_AvoidSearchRendering = { assets.Get<Texture2D>(AssetName::TexUIAvoidSearch), Colors::White };
		m_PositionRendering = { assets.Get<Texture2D>(AssetName::TexUIPosition), Colors::White };

		auto editCallback = [&](const MouseEvent& evt) {

			if (auto stageCoordsOpt = ScreenToStage({ evt.X, evt.Y }); stageCoordsOpt.has_value())
			{
				const auto& stageCoords = stageCoordsOpt.value();

				if (evt.Button == MouseButton::Left && !GetApplication().GetKeyPressed(GLFW_KEY_SPACE))
				{
					if (auto obj = s_toolMap.find(ActiveTool); obj != s_toolMap.end())
					{
						m_Stage->SetValueAt(stageCoords.x, stageCoords.y, obj->second);
					}
					else if (ActiveTool == StageEditorTool::AvoidSearch)
					{
						m_Stage->SetAvoidSearchAt(stageCoords.x, stageCoords.y, EAvoidSearch::Yes);
					}
					else if (ActiveTool == StageEditorTool::Position)
					{
						if (m_Stage->StartPoint == stageCoords)
							m_Stage->StartDirection = { -m_Stage->StartDirection.y, m_Stage->StartDirection.x };
						
						m_Stage->StartPoint = stageCoords;

					}

				}
				else if (evt.Button == MouseButton::Right)
				{
					m_Stage->SetValueAt(stageCoords.x, stageCoords.y, EStageObject::None);
					m_Stage->SetAvoidSearchAt(stageCoords.x, stageCoords.y, EAvoidSearch::No);
				}
			}
		};

		AddSubscription(MouseMoved, [&](const MouseEvent& evt) {
			m_CursorPos = ScreenToStage({ evt.X,evt.Y });
		});
		AddSubscription(MouseClicked, editCallback);
		AddSubscription(MouseDragged, editCallback);
		AddSubscription(MouseDragged, [&](const MouseEvent& evt) {
			if (evt.Button == MouseButton::Middle || (evt.Button == MouseButton::Left && GetApplication().GetKeyPressed(GLFW_KEY_SPACE)))
				m_ViewOrigin -= glm::vec2(evt.DeltaX, evt.DeltaY) / m_Zoom;
		});


		AddSubscription(Wheel, [&](const WheelEvent& evt) {
			const float m = evt.DeltaY > 0 ? m_ZoomStep : (1.0f / m_ZoomStep);
			m_TargetZoom = std::clamp(m_TargetZoom * m, m_MinZoom, m_MaxZoom);
		});


	}

	void UIStageEditorArea::Update(const UIRoot& root, const Time& time)
	{
		float zoomSpeed = std::max(0.5f, std::abs(m_Zoom - m_TargetZoom)) * 4.0f;
		auto prevPos = ScreenToWorld(root.GetMousePosition());
		m_Zoom = MoveTowards(m_Zoom, m_TargetZoom, time.Delta * zoomSpeed);
		auto nextPos = ScreenToWorld(root.GetMousePosition());

		m_ViewOrigin += prevPos - nextPos;

		// Update pattern texture
		UpdatePattern();

	}

	void UIStageEditorArea::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{

		if (m_Stage)
		{
			auto& style = GetStyle();
			auto& assets = Assets::GetInstance();
			auto size = glm::vec2(m_Stage->GetSize(), m_Stage->GetSize()) * m_Zoom;
			auto tiling = glm::vec2(m_Stage->GetSize(), m_Stage->GetSize()) / 2.0f;
			auto texWhite = assets.Get<Texture2D>(AssetName::TexWhite);

			r2.Push();
			
			r2.Clip(Bounds);
			

			r2.Texture(m_BgPattern);
			r2.Color(style.GetBackgroundColor(*this, style.Palette.Background));
			r2.DrawQuad(Bounds.Position, Bounds.Size, Bounds.Size / (m_Zoom * 2.0f), m_ViewOrigin / 2.0f + glm::vec2(0.25f));

			r2.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
			r2.Texture(m_Pattern);
			r2.DrawQuad(WorldToScreen({ 0, 0 }), size, tiling, { -0.25f, -0.25f });


			glm::uvec2 bl = glm::clamp(glm::floor(ScreenToWorld(Bounds.Position)), 0.0f, (float)m_Stage->GetSize());
			glm::uvec2 tr = glm::clamp(glm::ceil(ScreenToWorld(Bounds.Position + Bounds.Size)), 0.0f, (float)m_Stage->GetSize());

			for (uint32_t x = bl.x; x < tr.x; ++x)
			{
				for (uint32_t y = bl.y; y < tr.y; ++y)
				{

					if (auto obj = m_StageObjRendering.find(m_Stage->GetValueAt(x, y)); obj != m_StageObjRendering.end())
					{
						r2.Texture(std::get<0>(obj->second));

						r2.Color(style.ShadowColor);
						r2.DrawQuad(WorldToScreen({ x + style.ShadowOffset, y - style.ShadowOffset }), { m_Zoom, m_Zoom });
						
						r2.Color(std::get<1>(obj->second));
						r2.DrawQuad(WorldToScreen({ x, y }), { m_Zoom, m_Zoom });
					}

					if (m_Stage->GetAvoidSearchAt(x, y) == EAvoidSearch::Yes)
					{
						r2.Push();
						r2.Texture(std::get<0>(m_AvoidSearchRendering));
						r2.Color(style.ShadowColor);
						r2.DrawQuad(WorldToScreen({ x + style.ShadowOffset, y - style.ShadowOffset }), { m_Zoom, m_Zoom });

						r2.Color(std::get<1>(m_AvoidSearchRendering));
						r2.DrawQuad(WorldToScreen({ x, y }), { m_Zoom, m_Zoom });

						r2.Pop();
					}

				}
			}

			// Position
			{
				float angle = std::atan2f((float)m_Stage->StartDirection.y, (float)m_Stage->StartDirection.x)
					- glm::pi<float>() / 2.0f;

				r2.Push();
				r2.Texture(std::get<0>(m_PositionRendering));
				r2.Pivot(EPivot::Center);
				r2.Translate(WorldToScreen((glm::vec2)m_Stage->StartPoint + glm::vec2(0.5f)));
				r2.Scale(m_Zoom);
				{
					r2.Push();
					r2.Color(style.ShadowColor);
					r2.Translate({ style.ShadowOffset, -style.ShadowOffset });
					r2.Scale(1.25f + std::sin(time.Elapsed * 10.0f) * 0.25f);
					r2.Rotate(angle);
					r2.DrawQuad();
					r2.Pop();
				}

				r2.Scale(1.25f + std::sin(time.Elapsed * 10.0f) * 0.25f);
				r2.Rotate(angle);
				r2.Color(std::get<1>(m_PositionRendering));
				r2.DrawQuad();
				r2.Pop();
			}

			DrawCursor(r2, style);

			
			r2.Pop();
		}
	}

	void UIStageEditorArea::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UIStageEditorArea::UpdatePattern()
	{
		if (m_Stage)
		{
			m_Pattern = CreateCheckerBoard({
				ToHexColor(m_Stage->PatternColors[0]),
				ToHexColor(m_Stage->PatternColors[1])
			}, m_Pattern);
		}

	}

	
	void UIStageEditorArea::DrawCursor(Renderer2D& r2, const UIStyle& style)
	{
		const float lw = style.StageAreaCrosshairSize; // line width
		const float lt = style.StageAreaCrosshairThickness; // thickness
		const std::array<std::pair<glm::vec2, glm::vec2>, 8> quads = {
			std::make_pair(glm::vec2(0, 0), glm::vec2(lw, lt)),
			std::make_pair(glm::vec2(0, 0), glm::vec2(lt, lw)),
			std::make_pair(glm::vec2(1.0f - lw, 0), glm::vec2(lw, lt)),
			std::make_pair(glm::vec2(1.0f - lt, 0), glm::vec2(lt, lw)),
			std::make_pair(glm::vec2(1.0f - lw, 1.0f - lt), glm::vec2(lw, lt)),
			std::make_pair(glm::vec2(1.0f - lt, 1.0f - lw), glm::vec2(lt, lw)),
			std::make_pair(glm::vec2(0, 1.0f - lt), glm::vec2(lw, lt)),
			std::make_pair(glm::vec2(0, 1.0f - lw), glm::vec2(lt, lw)),
		};

		if (Hovered && m_CursorPos != std::nullopt)
		{
			r2.Push();
			r2.Translate(WorldToScreen(m_CursorPos.value()));
			r2.Scale(m_Zoom);
			r2.NoTexture();
			{ // Shadow
				r2.Push();
				r2.Translate({ style.ShadowOffset, -style.ShadowOffset });
				r2.Color(Colors::Black);
				for (const auto& [pos, size] : quads)
					r2.DrawQuad(pos, size);
				r2.Pop();
			}

			r2.Color(style.Palette.Secondary);
			for (const auto& [pos, size] : quads)
				r2.DrawQuad(pos, size);

			r2.Pop();
		}
	}

	glm::vec2 bsf::UIStageEditorArea::WorldToScreen(const glm::vec2 pos) const
	{
		return Bounds.Position + (pos - m_ViewOrigin) * m_Zoom;
	}

	glm::vec2 bsf::UIStageEditorArea::ScreenToWorld(const glm::vec2 pos) const
	{
		return m_ViewOrigin + (pos - Bounds.Position) / m_Zoom;
	}

	std::optional<glm::ivec2> UIStageEditorArea::ScreenToStage(const glm::vec2 screenPos) const
	{
		glm::ivec2 localPos = glm::floor(ScreenToWorld(screenPos));

		if (m_Stage != nullptr && localPos.x >= 0 && localPos.x < m_Stage->GetSize() 
			&& localPos.y >= 0 && localPos.y < m_Stage->GetSize())
			return localPos;
		else
			return std::nullopt;

	}

	UITextInput::UITextInput() : UIElement(MakeFlags(UIElementFlags::ReceiveFocus))
	{
		static std::string s_TestStr = " ";
		static std::regex s_AllowedCharacters("^[a-zA-Z0-9_\\s\\-]$");

		Margin = 1.0f;
		MinSize = { 0.0f, 1.5f };
		PreferredSize = { 0.0f, 1.5f };


		AddSubscription(KeyPressed, [&](const KeyPressedEvent& evt) {
			auto current = m_Value.Get();

			if (evt.KeyCode == GLFW_KEY_BACKSPACE) // Backspace
			{
				if (!current.empty())
					m_Value.Set(current.substr(0, current.size() - 1));
			}
		});

		AddSubscription(CharacterTyped, [&](const CharacterTypedEvent& evt) {
			auto current = m_Value.Get();
			s_TestStr[0] = evt.Character;
			if(std::regex_match(s_TestStr, s_AllowedCharacters))
				m_Value.Set(current + evt.Character);
		});

	}

	void UITextInput::Update(const UIRoot& root, const Time& time)
	{
		const float margin = GetStyle().GetMargin(Margin);

		// Update the current value
		MinSize.y = MaxSize.y = PreferredSize.y = 1.0f + GetStyle().LabelFontScale + 2.0f * margin;
	}

	void UITextInput::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UITextInput::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& style = GetStyle();
		auto value = m_Value.Get();
		const float margin = style.GetMargin(Margin);
		Rect innerBounds = Bounds;
		innerBounds.Shrink(margin); 

		r2.Push();

		r2.Color(style.ShadowColor);
		r2.DrawQuad(innerBounds.Position + glm::vec2(style.ShadowOffset, -style.ShadowOffset), innerBounds.Size);

		r2.Color(style.GetBackgroundColor(*this, style.Palette.BackgroundVariant));
		r2.DrawQuad(innerBounds.Position, innerBounds.Size);

		r2.Translate(innerBounds.Position);

		r2.Color(Focused ? style.Palette.Secondary : style.GetForegroundColor(*this, style.Palette.Foreground));

		// Text
		{
			r2.Push();
			r2.Pivot(EPivot::BottomLeft);
			r2.Translate({ margin, 0.0f });
			r2.DrawStringShadow(Assets::GetInstance().Get<Font>(AssetName::FontMain), value);
			r2.Pop();
		}

		// Label
		{
			r2.Push();

			r2.Pivot(EPivot::BottomLeft);

			r2.Translate({ margin, 1.0f });
			r2.Scale({ style.LabelFontScale, style.LabelFontScale });
			r2.DrawStringShadow(Assets::GetInstance().Get<Font>(AssetName::FontMain), Label);
			
			r2.Pop();
		}


		r2.Pop();
	}

	const glm::vec4 UIStyle::DefaultColor(const std::optional<glm::vec4>& colorOpt, const glm::vec4& default) const { return colorOpt.has_value() ? colorOpt.value() : default; }
	const glm::vec4 UIStyle::GetBackgroundColor(const UIElement& element, const glm::vec4& default) const { return DefaultColor(element.BackgroundColor, default); }
	const glm::vec4 UIStyle::GetForegroundColor(const UIElement& element, const glm::vec4& default) const { return DefaultColor(element.ForegroundColor, default); }

	void UIStyle::Recompute()
	{
		MarginUnit = GlobalScale / 100.0f;
		ShadowOffset = GlobalScale / 200.0f;
		SliderTrackThickness = GlobalScale / 300.0f;
		ToastWidth = GlobalScale / 3.0f;
	}

	void UIText::Update(const UIRoot& root, const Time& time)
	{
		auto& style = GetStyle();
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		const float margin = style.GetMargin(Margin);
		float width = font->GetStringWidth(Text);
		MaxSize = PreferredSize = MinSize = glm::vec2(width + 2.0f * margin, 1.0f + 2.0 * margin) * Scale;
	}

	void UIText::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UIText::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& style = GetStyle();
		const float margin = style.GetMargin();
		auto contentBounds = Bounds;
		contentBounds.Shrink(margin);

		r2.Push();
		r2.Pivot(EPivot::BottomLeft);
		r2.Translate(contentBounds.Position);
		r2.Scale(Scale);
		r2.Color(style.GetForegroundColor(*this, style.Palette.Foreground));
		r2.DrawStringShadow(Assets::GetInstance().Get<Font>(AssetName::FontMain), Text);
		r2.Pop();
	}





	UISlider::UISlider()
	{
		auto sliderMouseHandler = [&](const MouseEvent& evt) {
			auto contentBounds = GetContentBounds();
			glm::vec2 pos = { evt.X, evt.Y };
			auto conj = m_Limits[1] - m_Limits[0];
			float len = glm::length(conj);
			auto dir = glm::normalize(conj);
			float t = std::clamp(glm::dot(dir, pos - m_Limits[0]), 0.0f, len) / len;
			m_Value.Set(t * (Max - Min) + Min);
			ValueChanged.Emit({ m_Value.Get() });
		};

		AddSubscription(MouseDragged, sliderMouseHandler);
		AddSubscription(MousePressed, sliderMouseHandler);

	}

	void UISlider::Update(const UIRoot& root, const Time& time)
	{
		const auto& style = GetStyle();
		const float height = style.GetMargin(4.0f);
		MinSize = glm::vec2(height);
		if (Orientation == UISliderOrientation::Horizontal)
			MaxSize = { std::numeric_limits<float>::max(), height };
		else
			MaxSize = { height, std::numeric_limits<float>::max() };
	}

	void UISlider::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& assets = Assets::GetInstance();
		auto& style = GetStyle();
		auto texture = assets.Get<Texture2D>(AssetName::TexUISphere);
		auto contentBounds = GetContentBounds();
		float margin = style.GetMargin();

		auto color = style.DefaultColor(ForegroundColor, style.Palette.Primary);
		auto darker = Darken(color, 0.5f);

		r2.Push();

		glm::vec2 originOffset,
			trackSize,
			handleSize;
		EPivot trackPivot;


		if (Orientation == UISliderOrientation::Horizontal)
		{
			originOffset = { 0.0f, contentBounds.Size.y / 2.0f };
			trackSize = { contentBounds.Width(), style.SliderTrackThickness };
			handleSize = glm::vec2(contentBounds.Height() + margin);
			trackPivot = EPivot::Left;
		}
		else
		{
			originOffset = { contentBounds.Width() / 2.0f, 0.0f };
			trackSize = { style.SliderTrackThickness, contentBounds.Height() };
			handleSize = glm::vec2(contentBounds.Width() + margin);
			trackPivot = EPivot::Bottom;
		}

		auto handlePosition = GetHandlePosition();

		r2.Push();

		r2.Pivot(trackPivot);
		r2.NoTexture();
		r2.Color(darker);
		r2.DrawQuad(contentBounds.Position + originOffset, trackSize);

		r2.Pivot(EPivot::Center);

		r2.Push();
		r2.Translate({ style.ShadowOffset, -style.ShadowOffset });
		r2.Color(style.ShadowColor);
		r2.Texture(texture);
		r2.DrawQuad(handlePosition, handleSize);
		r2.Pop();

		r2.Color(color);
		r2.Texture(texture);
		r2.DrawQuad(handlePosition, handleSize);

		if (ShowValue && Hovered)
		{
			std::string display = m_Value.Format ? std::move(m_Value.Format()) : std::to_string(m_Value.Get());
			auto font = assets.Get<Font>(AssetName::FontMain);
			r2.Pivot(EPivot::Center);
			r2.Color(Colors::White);
			r2.Translate(handlePosition);
			r2.Scale(style.LabelFontScale);
			r2.DrawStringShadow(font, display);
		}


		r2.Pop();

	}

	void UISlider::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };

		auto contentBounds = GetContentBounds();

		if (Orientation == UISliderOrientation::Horizontal)
		{
			m_Limits[0] = { contentBounds.Left(), contentBounds.Bottom() + contentBounds.Height() / 2.0f };
			m_Limits[1] = { contentBounds.Right(), contentBounds.Bottom() + contentBounds.Height() / 2.0f };
		}
		else
		{
			m_Limits[0] = { contentBounds.Left() + contentBounds.Width() / 2.0f, contentBounds.Top() };
			m_Limits[1] = { contentBounds.Left() + contentBounds.Width() / 2.0f, contentBounds.Bottom() };
		}


	}

	Rect UISlider::GetContentBounds() const
	{
		auto& style = GetStyle();
		float margin = style.GetMargin();
		auto contentBounds = Bounds;
		if(Orientation == UISliderOrientation::Horizontal)
			contentBounds.Shrink(2.0f * margin, margin);
		else
			contentBounds.Shrink(margin, 2.0f * margin);

		return contentBounds;
	}

	bool UISlider::IntersectHandle(const glm::vec2& pos) const
	{
		auto contentBounds = GetContentBounds();

		float handleRadius = contentBounds.Size.y / 2.0f;
		glm::vec2 handlePos = { GetDelta() * contentBounds.Width() + contentBounds.Left(),
			contentBounds.Bottom() + contentBounds.Height() / 2.0f };

		return glm::distance(pos, handlePos) <= handleRadius;

	}

	float UISlider::GetDelta() const
	{
		return (m_Value.Get() - Min) / (Max - Min);
	}

	glm::vec2 UISlider::GetHandlePosition() const
	{
		return glm::lerp(m_Limits[0], m_Limits[1], GetDelta());
	}

	void UIColorPicker::Bind(const BindFn& fn)
	{
		m_GetValue = fn;
	}

	UIColorPicker::UIColorPicker()
	{
		Layout = UILayout::Vertical;


		m_RGB = {
			MakeRef<UISlider>(),
			MakeRef<UISlider>(),
			MakeRef<UISlider>()
		};

		m_RGB[0]->ForegroundColor = Colors::Red;
		m_RGB[1]->ForegroundColor = Colors::Green;
		m_RGB[2]->ForegroundColor = Colors::Blue;

		for (size_t i = 0; i < m_RGB.size(); ++i)
		{
			UIBoundValue<float> val = 0.0f;
			val.Get = [&, i] { return glm::value_ptr(m_GetValue())[i]; };
			val.Set = [&, i] (const float& v) { glm::value_ptr(m_GetValue())[i] = v; };
			m_RGB[i]->Bind(val);
		}

		m_Label = MakeRef<UIText>();

		AddChild(m_Label);

		for (const auto& slider : m_RGB)
			AddChild(slider);

		HasShadow = true;
		Margin = 1.0f;
	}

	void UIColorPicker::Update(const UIRoot& root, const Time& time)
	{
		auto& style = GetStyle();
		m_Label->Scale = style.LabelFontScale;
		m_Label->Text = Label;
		BackgroundColor = glm::vec4(m_GetValue(), 1.0f);

		UIPanel::Update(root, time);
	}


	UILayer::UILayer()
	{
		BackgroundColor = Colors::Transparent;
	}

	UIButton::UIButton()
	{
		Margin = 1.0f;
	}

	void UIButton::Update(const UIRoot& root, const Time& time)
	{
		auto& style = GetStyle();
		float margin = style.GetMargin(Margin);
		const auto& font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		MinSize = glm::vec2(font->GetStringWidth(Label) + 4.0f * margin, 1.0f + 2.0f * margin) * 1.0f;
	}

	void UIButton::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UIButton::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& style = GetStyle();
		float margin = style.GetMargin(Margin);
		const auto& font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		auto contentBounds = Bounds;
		contentBounds.Shrink(margin);

		r2.Push();

		r2.Translate(contentBounds.Position);

		r2.Color(style.ShadowColor);
		r2.DrawQuad({ style.ShadowOffset, -style.ShadowOffset }, contentBounds.Size);

		r2.Color(style.GetBackgroundColor(*this, style.Palette.BackgroundVariant));
		r2.DrawQuad({ 0.0f, 0.0f }, contentBounds.Size);

		r2.Pivot(EPivot::Center);
		r2.Translate(contentBounds.Size / 2.0f);
		r2.Color(Hovered ? style.Palette.Secondary : style.GetForegroundColor(*this, style.Palette.Foreground));
		r2.DrawStringShadow(font, Label);

		r2.Pop();

	}




	#pragma endregion

	UIStageList::UIStageList(uint32_t rows, uint32_t cols) :
		m_Rows(rows),
		m_Columns(cols)
	{
		Margin = 1.0f;

		AddSubscription(Wheel, [&](const WheelEvent& evt) {
			int32_t dir = -Sign(evt.DeltaY);
			m_HeightOffset = std::clamp(m_HeightOffset + dir * ItemHeight, 0.0f, m_MaxHeightOffset);
		});

		AddSubscription(MousePressed, [&](const MouseEvent& evt) {

			if (evt.Button == MouseButton::Left)
			{
				glm::vec2 pos = { evt.X,evt.Y };

				for (auto& info : m_StagesInfo)
				{
					if (!info.Visible)
						continue;

					if (info.CurrentBounds.Contains(pos))
					{
						info.IsDragged = true;
						m_DraggedItem = info;
					}
				}

				
			}
		});

		AddSubscription(MouseDragged, [&](const MouseEvent& evt) {
			if (evt.Button == MouseButton::Left && m_DraggedItem.has_value())
			{
				
				auto& draggedItem = m_DraggedItem.value();
				draggedItem.CurrentBounds.Position += glm::vec2(evt.DeltaX, evt.DeltaY);

				auto draggedIt = std::find(m_StagesInfo.begin(), m_StagesInfo.end(), draggedItem);

				for (auto it = m_StagesInfo.begin(); it != m_StagesInfo.end(); ++it)
				{
					if (it->TargetBounds.Contains({ evt.X, evt.Y }) && *it != *draggedIt)
					{
						
						std::vector<StageInfo> newStageInfo;
						newStageInfo.reserve(m_StagesInfo.size());
						auto [min, max] = std::minmax(it, draggedIt);
						auto count = max - min;
						auto inserter = std::copy(m_StagesInfo.begin(), min, std::back_inserter(newStageInfo));

						if (count == 1)
						{
							inserter = *max;
							inserter = *min;
							std::copy(max + 1, m_StagesInfo.end(), inserter);
						}
						else if (it > draggedIt)
						{
							inserter = std::copy(min + 1, max, inserter);
							inserter = *draggedIt;
							std::copy(max, m_StagesInfo.end(), inserter);
						}
						else
						{
							inserter = *draggedIt;
							inserter = std::copy(min, max, inserter);
							std::copy(max + 1, m_StagesInfo.end(), inserter);
						}
						
						m_StagesInfo = std::move(newStageInfo);

						// Must break here because we could potentially go over multiple boxes
						break;


					}
				}
			
			}
		});

		AddSubscription(GlobalMouseReleased, [&](const MouseEvent& evt) {
			if (m_DraggedItem.has_value())
			{
				auto& draggedItem = m_DraggedItem.value();

				auto draggedIt = std::find(m_StagesInfo.begin(), m_StagesInfo.end(), draggedItem);

				draggedIt->IsDragged = false;
				draggedIt->CurrentBounds = draggedItem.CurrentBounds;
				m_DraggedItem = std::nullopt;

				std::vector<std::string> files;
				files.reserve(m_StagesInfo.size());

				std::transform(m_StagesInfo.begin(), m_StagesInfo.end(), std::back_inserter(files), [](const StageInfo& info) {
					return info.FileName;
				});

				FileReorder.Emit({ std::move(files) });

			}
		});

		AddSubscription(MouseDblClicked, [&](const MouseEvent& evt) {
			
			if (evt.Button == MouseButton::Left)
			{
				glm::vec2 pos = { evt.X,evt.Y };

				for (const auto& info : m_StagesInfo)
				{
					if (!info.Visible)
						continue;

					if (info.CurrentBounds.Contains(pos))
					{
						FileSelected.Emit({ info.FileName });
						// So here we have to break, it could happend that
						// we click 2 files at the same time if they're animating.
						// We don't want that of course, a single click should
						// ba a single file
						break;
					}
				}
			}
		});

	}

	void UIStageList::SetFiles(const std::vector<std::string>& files)
	{
		m_Files = files;
		m_StagesInfo.clear();
		m_StagesInfo.resize(files.size());
		m_TotalRows = files.size() / m_Columns + (files.size() % m_Columns == 0 ? 0 : 1);
		m_MaxTopRow = std::max(0, (int32_t)m_TotalRows - (int32_t)m_Rows);
		m_HeightOffset = 0.0f;
	}

	void UIStageList::Update(const UIRoot& root, const Time& time)
	{
		auto& style = GetStyle();
		auto contentBounds = Bounds;
		const float margin = style.GetMargin(Margin);
		
		MinSize.y = m_Rows * (ItemHeight + margin) + margin;
		MinSize.x = m_Columns * (ItemWidth + margin) + margin;

		const float rowHeight = ItemHeight + margin;

		m_TotalHeight = m_TotalRows * rowHeight  + margin;
		m_MaxHeightOffset = m_MaxTopRow * rowHeight;

	}

	void UIStageList::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		constexpr float speedFactor = 3.0f;
		auto& style = GetStyle();
		auto contentBounds = Bounds;
		const float margin = style.GetMargin(Margin);

		contentBounds.Shrink(margin);

		r2.Push();
		r2.Pivot(EPivot::BottomLeft);


		r2.Clip(Bounds);

		for (auto& info : m_StagesInfo)
		{

			float speed = std::max(Bounds.Width(), glm::distance(info.CurrentBounds.Position, info.TargetBounds.Position)) * speedFactor;

			info.CurrentBounds.Position = MoveTowards(info.CurrentBounds.Position, info.TargetBounds.Position, speed * time.Delta);
			info.CurrentBounds.Size = MoveTowards(info.CurrentBounds.Size, info.TargetBounds.Size, speed);

			if (!(info.Loaded && info.Visible))
				continue;

			if(!info.IsDragged)
				RenderItem(r2, info);

		}

		r2.NoClip();

		if(m_DraggedItem.has_value())
			RenderItem(r2, m_DraggedItem.value());


		r2.Pop();
	}

	void UIStageList::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		// TODO can be dynamic doesn't really make any difference (not sure)
		// Or could ma make it static
		static Stage s_Stage;

		Bounds = { origin, computedSize };
		auto& style = GetStyle();
		auto contentBounds = Bounds;
		const float margin = style.GetMargin(Margin);
		contentBounds.Shrink(margin);

		ItemWidth = (contentBounds.Width() - (m_Columns - 1.0f) * margin) / m_Columns;

		std::for_each(m_StagesInfo.begin(), m_StagesInfo.end(), [](StageInfo& info) {info.Visible = false; });


		for (int32_t i = 0; i < m_Files.size(); ++i)
		{
			int32_t x = i % (int32_t)m_Columns;
			int32_t y = -i / (int32_t)m_Columns;

			auto& info = m_StagesInfo[i];

			info.TargetBounds.Position.x = contentBounds.Position.x + x * (ItemWidth + margin);
			info.TargetBounds.Position.y = contentBounds.Position.y + contentBounds.Height() +
				(y - 1) * (ItemHeight + margin) + margin + m_HeightOffset;

			info.TargetBounds.Size = { ItemWidth, ItemHeight };

			if (!info.Initialized)
			{
				info.CurrentBounds = info.TargetBounds;
				info.Initialized = true;
			}

			info.Visible = Bounds.Intersects(info.CurrentBounds);

			if (info.Visible && !info.Loaded)
			{
				s_Stage.Load(m_Files[i]);
				info.FileName = m_Files[i];
				info.Name = s_Stage.Name;
				info.MaxRings = s_Stage.MaxRings;
				info.BlueSpheres = s_Stage.Count(EStageObject::BlueSphere);
				info.Pattern = CreateCheckerBoard({
					ToHexColor(s_Stage.PatternColors[0]),
					ToHexColor(s_Stage.PatternColors[1])
					});
				info.Loaded = true;
			}

		}

	}

	void UIStageList::SetScroll(float scroll)
	{
		m_HeightOffset = glm::lerp(0.0f, m_MaxHeightOffset, std::clamp(scroll, 0.0f, 1.0f));
	}

	float UIStageList::GetScroll() const
	{
		return m_HeightOffset / m_MaxHeightOffset;
	}

	void UIStageList::RenderItem(Renderer2D& r2, const StageInfo& info)
	{
		auto& assets = Assets::GetInstance();
		auto& style = GetStyle();

		auto font = assets.Get<Font>(AssetName::FontMain);
		auto sphereUi = assets.Get<Texture2D>(AssetName::TexUISphere);
		auto ringUi = assets.Get<Texture2D>(AssetName::TexUIRing);

		const float margin = style.GetMargin(Margin);

		Rect innerBounds = info.CurrentBounds;
		innerBounds.Shrink(margin);

		// Internal interface
		r2.Push();
		{
			r2.NoTexture();
			r2.Color(style.ShadowColor);
			r2.DrawQuad(info.CurrentBounds.Position + glm::vec2(style.ShadowOffset, -style.ShadowOffset), info.CurrentBounds.Size);

			r2.Texture(info.Pattern);
			r2.Color({ glm::vec3(Colors::White * 0.75f), 1.0f });
			r2.DrawQuad(info.CurrentBounds.Position, info.CurrentBounds.Size, { 2.0f * info.CurrentBounds.Aspect(), 2.0f });

			r2.Clip(innerBounds);

			{ // Stage Name
				r2.Push();
				r2.Translate({ innerBounds.Left(), innerBounds.Bottom() });
				r2.Scale(style.LabelFontScale);
				r2.Color(style.GetForegroundColor(*this, style.Palette.Foreground));
				r2.DrawStringShadow(font, info.Name);
				r2.Pop();
			}

			{ // Sphere counter
				r2.Push();
				r2.Pivot(EPivot::TopLeft);
				r2.Translate({ innerBounds.Left(), innerBounds.Top() });
				r2.Scale(style.LabelFontScale);

				r2.Texture(sphereUi);

				r2.Color(style.ShadowColor);
				r2.DrawQuad({ style.ShadowOffset, -style.ShadowOffset });

				r2.Color(Colors::BlueSphere);
				r2.DrawQuad();

				r2.Color(style.GetForegroundColor(*this, style.Palette.Foreground));
				r2.DrawStringShadow(font, std::to_string(info.BlueSpheres), { 1.0f + margin, 0.0f });

				r2.Pop();
			}

			{ // Ring counter
				r2.Push();
				r2.Pivot(EPivot::TopRight);
				r2.Translate({ innerBounds.Right(), innerBounds.Top() });
				r2.Scale(style.LabelFontScale);

				r2.Texture(ringUi);

				r2.Color(style.ShadowColor);
				r2.DrawQuad({ style.ShadowOffset, -style.ShadowOffset });

				r2.Color(Colors::Ring);
				r2.DrawQuad();

				r2.Color(style.GetForegroundColor(*this, style.Palette.Foreground));
				r2.DrawStringShadow(font, std::to_string(info.MaxRings), { -1.0f - margin, 0.0f });

				r2.Pop();
			}
		}
		r2.Pop();

	}

}