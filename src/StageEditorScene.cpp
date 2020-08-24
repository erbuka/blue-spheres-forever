#include "BsfPch.h"

#include <regex>

#include "StageEditorScene.h"
#include "Assets.h"
#include "Texture.h"
#include "Renderer2D.h"
#include "Application.h"
#include "MenuScene.h"
#include "Font.h"


namespace bsf
{
	static constexpr float s_uiScale = 15.0f;
	static constexpr float s_uiPanelMargin = s_uiScale / 100.0f;
	static constexpr float s_uiTopBarHeight = s_uiScale / 10.0f;
	static constexpr float s_uiPropertiesWidth = s_uiScale / 2.0f;
	static constexpr float s_uiToolbarButtonSize = s_uiTopBarHeight - 2 * s_uiPanelMargin;
	static constexpr float s_uiSaveDialogSize = s_uiScale / 2.0f;
	static constexpr float s_uiStageListDialogSize = s_uiScale;

	void StageEditorScene::OnAttach()
	{
		m_CurrentStage = MakeRef<Stage>();
		/*
		for (uint32_t x = 0; x < m_CurrentStage->GetWidth(); x++)
			for (uint32_t y = 0; y < m_CurrentStage->GetHeight(); y++)
				m_CurrentStage->SetValueAt(x, y, EStageObject::BlueSphere);
		*/
		InitializeUI();
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
	}

	void StageEditorScene::OnSaveButtonClick(const MouseEvent& evt)
	{
		if (m_CurrentStageFile.has_value())
		{
			m_CurrentStage->Save(m_CurrentStageFile.value());
		}
		else
		{
			m_uiSaveStageName->SetValue(m_CurrentStage->Name);
			m_uiRoot->PushLayer(m_uiSaveDialogLayer);
		}
	}

	void StageEditorScene::OnStageListButtonClick(const MouseEvent& evt)
	{
		auto files = Stage::GetStageFiles();
		m_uiStageList->SetFiles(std::move(files));
		m_uiRoot->PushLayer(m_uiStageListDialogLayer);
	}

	void StageEditorScene::LoadStage(const std::string& fileName)
	{
		// TODO Add error checking maybe
		m_CurrentStage->Load(fileName);
		m_CurrentStageFile = fileName;
	}

	void StageEditorScene::InitializeUI()
	{
		auto& assets = Assets::GetInstance();

		UIStyle style;
		style.GlobalScale = s_uiScale;

		m_uiRoot = MakeRef<UIRoot>();
		m_uiRoot->Attach(GetApplication());
		m_uiRoot->SetStyle(style);
		
		// Editor layer
		{
			// Top Bar
			auto topBar = MakeRef<UIPanel>();
			topBar->Layout = UILayout::Horizontal;
			topBar->PreferredSize = { 0.0f, s_uiTopBarHeight };

			auto title = MakeRef<UIText>();
			title->Text = "Stage Editor";
			topBar->AddChild(title);

			auto tools = {
				std::make_tuple(StageEditorTool::BlueSphere, assets.Get<Texture2D>(AssetName::TexSphereUI), Colors::BlueSphere),
				std::make_tuple(StageEditorTool::RedSphere, assets.Get<Texture2D>(AssetName::TexSphereUI), Colors::RedSphere),
				std::make_tuple(StageEditorTool::YellowSphere, assets.Get<Texture2D>(AssetName::TexSphereUI), Colors::YellowSphere),
				std::make_tuple(StageEditorTool::Bumper, assets.Get<Texture2D>(AssetName::TexSphereUI), Colors::White),
				std::make_tuple(StageEditorTool::Ring, assets.Get<Texture2D>(AssetName::TexRingUI), Colors::Ring),
			};

			for (const auto& toolSpec : tools)
			{
				auto btn = MakeRef<UIIconButton>();

				btn->PreferredSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
				btn->MaxSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
				btn->Icon = std::get<1>(toolSpec);
				btn->Tint = std::get<2>(toolSpec);
				auto tool = std::get<0>(toolSpec);

				AddSubscription(btn->MouseClicked, [&, btn, tool](const MouseEvent& evt) {
					for (auto& b : m_ToolButtons)
						b->Selected = false;
					m_uiEditorArea->ActiveTool = tool;
					btn->Selected = true;
				});

				m_ToolButtons.push_back(btn);
				topBar->AddChild(btn);
			}

			// Save button
			{
				auto btn = MakeRef<UIIconButton>();
				btn->PreferredSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
				btn->MaxSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
				btn->Icon = assets.Get<Texture2D>(AssetName::TexWhite);
				btn->Tint = Colors::Green;
				AddSubscription(btn->MouseClicked, this, &StageEditorScene::OnSaveButtonClick);
				topBar->AddChild(btn);
			}

			// Stage list button
			{
				auto btn = MakeRef<UIIconButton>();
				btn->PreferredSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
				btn->MaxSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
				btn->Icon = assets.Get<Texture2D>(AssetName::TexWhite);
				btn->Tint = Colors::Yellow;
				AddSubscription(btn->MouseClicked, this, &StageEditorScene::OnStageListButtonClick);
				topBar->AddChild(btn);

			}

			// Properties
			auto properties = MakeRef<UIPanel>();
			properties->Layout = UILayout::Vertical;
			properties->PreferredSize = { s_uiPropertiesWidth, 0.0f };

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

			// Sky Colors
			{

				auto panel = MakeRef<UIPanel>();
				panel->Layout = UILayout::Horizontal;

				auto cp1 = MakeRef<UIColorPicker>();
				cp1->PreferredSize.x = s_uiPropertiesWidth / 2.0f;
				cp1->Bind([&]() -> glm::vec3& { return m_CurrentStage->SkyColors[0]; });
				cp1->Label = "Sky Primary";
				panel->AddChild(cp1);

				auto cp2 = MakeRef<UIColorPicker>();
				cp2->PreferredSize.x = s_uiPropertiesWidth / 2.0f;
				cp2->Bind([&]() -> glm::vec3& { return m_CurrentStage->SkyColors[1]; });
				cp2->Label = "Sky Secondary";

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

			// Max Rings input
			{
				auto maxRingsInput = MakeRef<UITextInput>();
				maxRingsInput->Label = "Max Rings";
				maxRingsInput->BindPush([&, check = std::regex("^[0-9]+$")](const std::string& val) {

					if (std::regex_match(val, check))
					{
						m_CurrentStage->MaxRings = std::atoi(val.c_str());
						return true;
					}
					else if (val.empty())
					{
						m_CurrentStage->MaxRings = 0;
						return true;
					}
					else
					{
						return false;
					}
				});

				maxRingsInput->BindPull([&] {return std::to_string(m_CurrentStage->MaxRings); });

				properties->AddChild(maxRingsInput);
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
		
		// Stage save dialog layer
		{
			m_uiSaveDialogLayer = MakeRef<UILayer>();
			m_uiSaveDialogLayer->Layout = UILayout::Fill;
			m_uiSaveDialogLayer->BackgroundColor = style.ShadowColor;
			AddSubscription(m_uiSaveDialogLayer->MouseClicked, [&](auto&) { m_uiRoot->PopLayer(); });

			{
				auto dialogPanel = MakeRef<UIPanel>();
				dialogPanel->Layout = UILayout::Vertical;
				dialogPanel->HasShadow = true;
				dialogPanel->PreferredSize.x = s_uiSaveDialogSize;
				dialogPanel->Pack();

				m_uiSaveStageName = MakeRef<UITextInput>();
				m_uiSaveStageName->Label = "Stage Name";
				dialogPanel->AddChild(m_uiSaveStageName);

				auto okBtn = MakeRef<UIButton>();
				okBtn->Label = "Save";


				AddSubscription(okBtn->MouseClicked, [&] (const MouseEvent& evt) {
					auto name = m_uiSaveStageName->GetValue();

					BSF_INFO("Name: {0}", name);

					if (!name.empty())
					{
						auto file = Format("assets/data/%s.jbss", name);
						m_CurrentStage->Save(file);
						m_CurrentStageFile = file;
						m_uiRoot->PopLayer();
					}
				});

				dialogPanel->AddChild(okBtn);

				m_uiSaveDialogLayer->AddChild(dialogPanel);
			}
		}

		// Stage list dialog layer
		{
			m_uiStageListDialogLayer = MakeRef<UILayer>();
			m_uiStageListDialogLayer->Layout = UILayout::Fill;
			m_uiStageListDialogLayer->BackgroundColor = style.ShadowColor;
			AddSubscription(m_uiStageListDialogLayer->MouseClicked, [&](auto&) { m_uiRoot->PopLayer(); });

			auto dialogPanel = MakeRef<UIPanel>();
			dialogPanel->Layout = UILayout::Vertical;
			dialogPanel->HasShadow = true;
			dialogPanel->PreferredSize.x = s_uiStageListDialogSize;
			dialogPanel->Pack();


			m_uiStageList = MakeRef<UIStageList>(4, 4);
			
			AddSubscription(m_uiStageList->FileSelected, [&](const UIStageList::StageSelectedEvent& evt) { 
				LoadStage(evt.FileName); 
				m_uiRoot->PopLayer();
			});
			
			AddSubscription(m_uiStageList->FileReorder, [&](const UIStageList::StageReorderEvent& evt) {
				Stage::SaveStageFiles(evt.Files);
			});
			
			dialogPanel->AddChild(m_uiStageList);

			m_uiStageListDialogLayer->AddChild(dialogPanel);

		}

	}


	#pragma region UI Impl


	void UIPanel::AddChild(const Ref<UIElement>& child)
	{
		m_Children.push_back(child);
	}

	void UIPanel::RemoveChild(const Ref<UIElement>& child)
	{

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
				std::pair<std::string, std::string>{ "Position", Format("%.2f %.2f", Bounds.Left(), Bounds.Top()) },
				std::pair<std::string, std::string>{ "Size", Format("%.2f %.2f", Bounds.Width(), Bounds.Height()) },
				std::pair<std::string, std::string>{ "Margin", Format("%.2f", Margin) }
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
				m_MouseState.HoverTarget->Wheel.Emit(evt);
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

				if (std::chrono::system_clock::now() - buttonState.TimePressed < std::chrono::milliseconds(250))
				{
					intersection->MouseClicked.Emit({ pos.x, pos.y, 0, 0, evt.Button });
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
		m_WindowSize = windowSize;
		m_Viewport = viewport;
		m_Projection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y, -1.0f, 1.0f);
		m_InverseProjection = glm::inverse(m_Projection);

		for (uint32_t i = 0; i < m_LayersToPop; ++i)
			m_Layers.pop_back();

		m_LayersToPop = 0;

		r2.Begin(m_Projection);
		for (auto& l = m_Layers.begin(); l != m_Layers.end(); l++)
		{
			(*l)->Traverse([&](UIElement& el) { el.m_Style = &m_Style; });
			(*l)->Update(*this, time);
			(*l)->UpdateBounds(*this, { 0.0f, 0.0f }, viewport);
			(*l)->Render(*this, r2, time);
#ifdef BSF_ENABLE_DIAGNOSTIC
			(*l)->Traverse([&](UIElement& el) { el.RenderDebugInfo(*this, r2, time); });
#endif
		}
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
			{ StageEditorTool::BlueSphere, EStageObject::BlueSphere },
			{ StageEditorTool::RedSphere, EStageObject::RedSphere },
			{ StageEditorTool::YellowSphere, EStageObject::YellowSphere },
			{ StageEditorTool::Bumper, EStageObject::Bumper },
			{ StageEditorTool::Ring, EStageObject::Ring }
		};

		m_StageObjRendering = {
			{ EStageObject::BlueSphere,		{ assets.Get<Texture2D>(AssetName::TexSphereUI),	Colors::BlueSphere }},
			{ EStageObject::RedSphere,		{ assets.Get<Texture2D>(AssetName::TexSphereUI),	Colors::RedSphere }},
			{ EStageObject::YellowSphere,	{ assets.Get<Texture2D>(AssetName::TexSphereUI),	Colors::YellowSphere }},
			{ EStageObject::Bumper,			{ assets.Get<Texture2D>(AssetName::TexSphereUI),	Colors::White }},
			{ EStageObject::Ring,			{ assets.Get<Texture2D>(AssetName::TexRingUI),		Colors::Ring }}
		};

		m_Pattern = CreateCheckerBoard({ 0, 0 });
		m_BgPattern = CreateCheckerBoard({ 0xffffffff, 0xffcccccc });

		auto editCallback = [&](const MouseEvent& evt) {

			if (auto stageCoordsOpt = GetStageCoordinates({ evt.X, evt.Y }); stageCoordsOpt.has_value())
			{
				const auto& stageCoords = stageCoordsOpt.value();

				if (evt.Button == MouseButton::Left)
				{
					if (auto obj = s_toolMap.find(ActiveTool); obj != s_toolMap.end())
						m_Stage->SetValueAt(stageCoords.x, stageCoords.y, obj->second);
				}

				if (evt.Button == MouseButton::Right)
				{
					m_Stage->SetValueAt(stageCoords.x, stageCoords.y, EStageObject::None);
					m_Stage->SetAvoidSearchAt(stageCoords.x, stageCoords.y, EAvoidSearch::No);
				}
			}
		};

		AddSubscription(MouseMoved, [&](const MouseEvent& evt) {
			m_CursorPos = GetStageCoordinates({ evt.X,evt.Y });
		});
		AddSubscription(MouseClicked, editCallback);
		AddSubscription(MouseDragged, editCallback);
		AddSubscription(MouseDragged, [&](const MouseEvent& evt) {
			if (evt.Button == MouseButton::Middle)
				m_ViewOrigin -= glm::vec2(evt.DeltaX, evt.DeltaY);
		});


		AddSubscription(Wheel, [&](const WheelEvent& evt) {
			const float m = evt.DeltaY > 0 ? 1.5f : (1.0f / 1.5f);
			m_TargetZoom = std::clamp(m_TargetZoom * m, m_MinZoom, m_MaxZoom);
		});


	}

	void UIStageEditorArea::Update(const UIRoot& root, const Time& time)
	{
		float zoomSpeed = std::max(0.5f, std::abs(m_Zoom - m_TargetZoom)) * 4.0f;
		m_Zoom = MoveTowards(m_Zoom, m_TargetZoom, time.Delta * zoomSpeed);

		// Update pattern texture
		UpdatePattern();

	}

	void UIStageEditorArea::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{

		if (m_Stage)
		{
			auto& style = GetStyle();

			auto size = glm::vec2(m_Stage->GetWidth(), m_Stage->GetHeight()) * m_Zoom;
			auto tiling = glm::vec2(m_Stage->GetWidth(), m_Stage->GetHeight()) / 2.0f;

			r2.Push();
			
			r2.Color(style.GetBackgroundColor(*this, style.Palette.Background));
			r2.DrawQuad(Bounds.Position, Bounds.Size);

			r2.Clip(Bounds);
			
			r2.Texture(m_BgPattern);
			r2.DrawQuad(Bounds.Position, Bounds.Size, Bounds.Size / (m_Zoom * 2.0f) , m_ViewOrigin / (m_Zoom * 2.0f) + glm::vec2(0.25f, 0.25f));

			r2.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
			r2.Texture(m_Pattern);
			r2.DrawQuad(Bounds.Position - m_ViewOrigin, size, tiling, { -0.25f, -0.25f });

			r2.Translate(Bounds.Position - m_ViewOrigin);
			r2.Scale({ m_Zoom, m_Zoom });

			glm::uvec2 bl = glm::clamp(glm::floor(GetRawStageCoordinates(Bounds.Position)), 0.0f, (float)m_Stage->GetWidth());
			glm::uvec2 tr = glm::clamp(glm::ceil(GetRawStageCoordinates(Bounds.Position + Bounds.Size)), 0.0f, (float)m_Stage->GetHeight());

			for (uint32_t x = bl.x; x < tr.x; ++x)
			{
				for (uint32_t y = bl.y; y < tr.y; ++y)
				{

					if (auto obj = m_StageObjRendering.find(m_Stage->GetValueAt(x, y)); obj != m_StageObjRendering.end())
					{
						r2.Texture(std::get<0>(obj->second));

						r2.Color(style.ShadowColor);
						r2.DrawQuad({ x + style.ShadowOffset, y - style.ShadowOffset });
						
						r2.Color(std::get<1>(obj->second));
						r2.DrawQuad({ x, y });
					}
				}
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
		static const float lw = style.StageAreaCrosshairSize; // line width
		static const float lt = style.StageAreaCrosshairThickness; // thickness
		static const std::array<std::pair<glm::vec2, glm::vec2>, 8> quads = {
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
			r2.Translate(m_CursorPos.value());
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

	glm::vec2 UIStageEditorArea::GetRawStageCoordinates(const glm::vec2 pos) const
	{
		return  (pos - Bounds.Position + m_ViewOrigin) / m_Zoom;
	}

	std::optional<glm::ivec2> UIStageEditorArea::GetStageCoordinates(const glm::vec2 pos) const
	{
		glm::ivec2 localPos = glm::floor(GetRawStageCoordinates(pos));

		if (m_Stage != nullptr && localPos.x >= 0 && localPos.x < m_Stage->GetWidth() 
			&& localPos.y >= 0 && localPos.y < m_Stage->GetHeight())
			return localPos;
		else
			return std::nullopt;

	}

	UITextInput::UITextInput() : UIElement(MakeFlags(UIElementFlags::ReceiveFocus))
	{
		Margin = 1.0f;
		MinSize = { 0.0f, 1.5f };
		PreferredSize = { 0.0f, 1.5f };

		AddSubscription(KeyPressed, [&](const KeyPressedEvent& evt) {
			if ((evt.KeyCode >= GLFW_KEY_A && evt.KeyCode <= GLFW_KEY_Z) ||
				(evt.KeyCode >= GLFW_KEY_0 && evt.KeyCode <= GLFW_KEY_9))
			{
				m_Push(m_Value + (char)evt.KeyCode);
			}
			else if (evt.KeyCode == GLFW_KEY_BACKSPACE && !m_Value.empty())
			{
				m_Push(m_Value.substr(0, m_Value.size() - 1));
			}
		});

	}

	void UITextInput::Update(const UIRoot& root, const Time& time)
	{
		const float margin = GetStyle().GetMargin(Margin);

		// Update the current value
		m_Value = m_Pull();
		MinSize.y = MaxSize.y = PreferredSize.y = 1.5f + 2.0f * margin;
	}

	void UITextInput::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UITextInput::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& style = GetStyle();

		const float margin = style.GetMargin(Margin);
		glm::vec2 shadowOffset = { style.TextShadowOffset, -style.TextShadowOffset };
		Rect innerBounds = Bounds;
		innerBounds.Shrink(margin); 

		r2.Push();

		r2.Color(style.ShadowColor);
		r2.DrawQuad(innerBounds.Position + glm::vec2(style.ShadowOffset, -style.ShadowOffset), innerBounds.Size);

		r2.Color(style.GetBackgroundColor(*this, style.Palette.BackgroundVariant));
		r2.DrawQuad(innerBounds.Position, innerBounds.Size);

		r2.TextShadowColor(style.ShadowColor);
		r2.TextShadowOffset(shadowOffset);

		r2.Translate(innerBounds.Position);

		r2.Color(Focused ? style.Palette.Secondary : style.GetForegroundColor(*this, style.Palette.Foreground));

		// Text
		{
			r2.Push();
			r2.Pivot(EPivot::BottomLeft);
			r2.Translate({ margin, 0.0f });
			r2.DrawStringShadow(Assets::GetInstance().Get<Font>(AssetName::FontMain), m_Value);
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



	inline const glm::vec4& UIStyle::DefaultColor(const std::optional<glm::vec4>& colorOpt, const glm::vec4& default) const { return colorOpt.has_value() ? colorOpt.value() : default; }

	inline const glm::vec4 UIStyle::GetBackgroundColor(const UIElement& element, const glm::vec4& default) const { return DefaultColor(element.BackgroundColor, default); }

	inline const glm::vec4 UIStyle::GetForegroundColor(const UIElement & element, const glm::vec4 & default) const { return DefaultColor(element.ForegroundColor, default); }

	void UIStyle::Recompute()
	{
		MarginUnit = GlobalScale / 100.0f;
		ShadowOffset = GlobalScale / 200.0f;
		SliderTrackThickness = GlobalScale / 300.0f;
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
		r2.TextShadowOffset({ style.TextShadowOffset, -style.TextShadowOffset });
		r2.TextShadowColor(style.ShadowColor);
		r2.Color(style.GetForegroundColor(*this, style.Palette.Foreground));
		r2.DrawStringShadow(Assets::GetInstance().Get<Font>(AssetName::FontMain), Text);
		r2.Pop();
	}





	UISlider::UISlider()
	{
		auto sliderMouseHandler = [&](const MouseEvent& evt) {
			auto contentBounds = GetContentBounds();
			float t = (std::clamp(evt.X, contentBounds.Left(), contentBounds.Right()) - contentBounds.Left()) / contentBounds.Width();
			m_GetValue() = t * (Max - Min) + Min;
		};

		AddSubscription(MouseDragged, sliderMouseHandler);
		AddSubscription(MousePressed, sliderMouseHandler);

	}

	void UISlider::Bind(const BindFn& fn)
	{
		m_GetValue = fn;
	}

	void UISlider::Update(const UIRoot& root, const Time& time)
	{
		auto& style = GetStyle();
		MinSize.y = MaxSize.y = style.SliderTrackThickness + style.GetMargin() * 4.0f;
	}

	void UISlider::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& style = GetStyle();
		auto texture = Assets::GetInstance().Get<Texture2D>(AssetName::TexSphereUI);
		auto contentBounds = GetContentBounds();
		float margin = style.GetMargin();

		auto color = style.DefaultColor(ForegroundColor, style.Palette.Primary);
		auto darker = Darken(color, 0.5f);

		r2.Push();

		r2.Translate(contentBounds.Position + glm::vec2(0.0f, contentBounds.Size.y / 2.0f));

		r2.Pivot(EPivot::Left);
		r2.NoTexture();
		r2.Color(darker);
		r2.DrawQuad({ 0.0f,0.0f }, { contentBounds.Size.x, style.SliderTrackThickness });

		r2.Pivot(EPivot::Center);
		
		r2.Push();
		r2.Translate({ style.ShadowOffset, -style.ShadowOffset });
		r2.Color(style.ShadowColor);
		r2.Texture(texture);
		r2.DrawQuad({ GetDelta() * contentBounds.Size.x, 0.0f }, glm::vec2(contentBounds.Height() + margin));
		r2.Pop();

		r2.Color(color);
		r2.Texture(texture);
		r2.DrawQuad({ GetDelta() * contentBounds.Size.x, 0.0f }, glm::vec2(contentBounds.Height() + margin));

		r2.Pop();

	}

	void UISlider::UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	Rect UISlider::GetContentBounds() const
	{
		auto& style = GetStyle();
		float margin = style.GetMargin();
		auto contentBounds = Bounds;
		contentBounds.Shrink(2.0f * margin, margin);
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
		return (m_GetValue() - Min) / (Max - Min);
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

		m_RGB[0]->Bind([&]() -> float& { return m_GetValue().r; });
		m_RGB[1]->Bind([&]() -> float& { return m_GetValue().g; });
		m_RGB[2]->Bind([&]() -> float& { return m_GetValue().b; });

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
		MinSize = glm::vec2(font->GetStringWidth(Label) + 2.0f * margin, 1.0f + 2.0f * margin) * 1.0f;
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
		r2.TextShadowColor(style.ShadowColor);
		r2.TextShadowOffset({ style.TextShadowOffset, -style.TextShadowOffset });
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
			//m_TopRow = std::clamp((int32_t)m_TopRow + dir, 0, (int32_t)m_MaxTopRow);
			m_HeightOffset = std::clamp(m_HeightOffset + dir * m_ItemHeight, 0.0f, m_MaxHeightOffset);

		});

		AddSubscription(MousePressed, [&](const MouseEvent& evt) {

			if (evt.Button == MouseButton::Left)
			{
				glm::vec2 pos = { evt.X,evt.Y };

				for (auto& info : m_FilesInfo)
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
			if (evt.Button == MouseButton::Left)
			{
				if (m_DraggedItem.has_value())
				{
					auto& draggedItem = m_DraggedItem.value();
					draggedItem.CurrentBounds.Position = { evt.X, evt.Y };

					auto draggedIt = std::find(m_FilesInfo.begin(), m_FilesInfo.end(), draggedItem);

					for (auto it = m_FilesInfo.begin(); it != m_FilesInfo.end(); ++it)
					{
						if (it->TargetBounds.Contains({ evt.X, evt.Y }))
						{
							std::swap(*it, *draggedIt);
						}
					}

					

				}
			
			}
		});

		AddSubscription(GlobalMouseReleased, [&](const MouseEvent& evt) {
			if (m_DraggedItem.has_value())
			{
				auto& draggedItem = m_DraggedItem.value();

				auto draggedIt = std::find(m_FilesInfo.begin(), m_FilesInfo.end(), draggedItem);

				draggedIt->IsDragged = false;
				draggedIt->CurrentBounds = draggedItem.CurrentBounds;
				m_DraggedItem = std::nullopt;

				std::vector<std::string> files;
				files.reserve(m_FilesInfo.size());

				std::transform(m_FilesInfo.begin(), m_FilesInfo.end(), std::back_inserter(files), [](const StageInfo& info) {
					return info.FileName;
				});

				FileReorder.Emit({ std::move(files) });

			}
		});

		AddSubscription(MouseClicked, [&](const MouseEvent& evt) {
			
			if (evt.Button == MouseButton::Left)
			{
				glm::vec2 pos = { evt.X,evt.Y };

				for (const auto& info : m_FilesInfo)
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
		uint32_t visibleItems = m_Rows * m_Columns;
		m_Files = files;
		m_FilesInfo.clear();
		m_FilesInfo.resize(files.size());
		m_TotalRows = files.size() / m_Columns + (files.size() % m_Columns == 0 ? 0 : 1);
		m_MaxTopRow = std::max(0, (int32_t)m_TotalRows - (int32_t)m_Rows);
		m_HeightOffset = 0.0f;
	}

	void UIStageList::Update(const UIRoot& root, const Time& time)
	{
		auto& style = GetStyle();
		auto contentBounds = Bounds;
		const float margin = style.GetMargin(Margin);
		
		m_ItemHeight = 2.0f;
		MinSize.y = m_Rows * (m_ItemHeight + margin) + margin;

		const float rowHeight = m_ItemHeight + margin;

		m_TotalHeight = m_TotalRows * rowHeight  + margin;
		m_MaxHeightOffset = m_MaxTopRow * rowHeight;

	}

	void UIStageList::Render(const UIRoot& root, Renderer2D& r2, const Time& time)
	{
		auto& style = GetStyle();
		auto contentBounds = Bounds;
		const float margin = style.GetMargin(Margin);
		const float dS = time.Delta * Bounds.Width();

		contentBounds.Shrink(margin);

		r2.Push();
		r2.Pivot(EPivot::BottomLeft);


		r2.Clip(Bounds);

		for (auto& info : m_FilesInfo)
		{

			info.CurrentBounds.Position = MoveTowards(info.CurrentBounds.Position, info.TargetBounds.Position, dS);
			info.CurrentBounds.Size = MoveTowards(info.CurrentBounds.Size, info.TargetBounds.Size, dS);

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
		// TODO can be dynamic doesn't really make any difference
		static Stage s_Stage;

		Bounds = { origin, computedSize };
		auto& style = GetStyle();
		auto contentBounds = Bounds;
		const float margin = style.GetMargin(Margin);
		contentBounds.Shrink(margin);

		m_ItemWidth = (contentBounds.Width() - (m_Columns - 1.0f) * margin) / m_Columns;

		std::for_each(m_FilesInfo.begin(), m_FilesInfo.end(), [](StageInfo& info) {info.Visible = false; });


		for (int32_t i = 0; i < m_Files.size(); ++i)
		{
			int32_t x = i % (int32_t)m_Columns;
			int32_t y = -i / (int32_t)m_Columns;

			auto& info = m_FilesInfo[i];

			info.TargetBounds.Position.x = contentBounds.Position.x + x * (m_ItemWidth + margin);
			info.TargetBounds.Position.y = contentBounds.Position.y + contentBounds.Height() +
				(y - 1) * (m_ItemHeight + margin) + margin + m_HeightOffset;

			info.TargetBounds.Size = { m_ItemWidth, m_ItemHeight };

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

	void UIStageList::RenderItem(Renderer2D& r2, const StageInfo& info)
	{
		auto& assets = Assets::GetInstance();
		auto& style = GetStyle();

		auto font = assets.Get<Font>(AssetName::FontMain);
		auto sphereUi = assets.Get<Texture2D>(AssetName::TexSphereUI);
		auto ringUi = assets.Get<Texture2D>(AssetName::TexRingUI);

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
				r2.Scale(0.75f);
				r2.Color(style.GetForegroundColor(*this, style.Palette.Foreground));
				r2.TextShadowColor(style.ShadowColor);
				r2.TextShadowOffset({ style.TextShadowOffset, -style.TextShadowOffset });
				r2.DrawStringShadow(font, info.Name);
				r2.Pop();
			}

			{ // Sphere counter
				r2.Push();
				r2.Pivot(EPivot::TopLeft);
				r2.Translate({ innerBounds.Left(), innerBounds.Top() });
				r2.Scale(0.5f);

				r2.Texture(sphereUi);

				r2.Color(style.ShadowColor);
				r2.DrawQuad({ style.ShadowOffset, -style.ShadowOffset });

				r2.Color(Colors::BlueSphere);
				r2.DrawQuad();

				r2.Color(style.GetForegroundColor(*this, style.Palette.Foreground));
				r2.TextShadowColor(style.ShadowColor);
				r2.TextShadowOffset({ style.TextShadowOffset, -style.TextShadowOffset });
				r2.DrawStringShadow(font, Format("%d", info.MaxRings), { 1.0f + margin, 0.0f });

				r2.Pop();
			}

			{ // Ring counter
				r2.Push();
				r2.Pivot(EPivot::TopRight);
				r2.Translate({ innerBounds.Right(), innerBounds.Top() });
				r2.Scale(0.5f);

				r2.Texture(ringUi);

				r2.Color(style.ShadowColor);
				r2.DrawQuad({ style.ShadowOffset, -style.ShadowOffset });

				r2.Color(Colors::Ring);
				r2.DrawQuad();

				r2.Color(style.GetForegroundColor(*this, style.Palette.Foreground));
				r2.TextShadowColor(style.ShadowColor);
				r2.TextShadowOffset({ style.TextShadowOffset, -style.TextShadowOffset });
				r2.DrawStringShadow(font, Format("%d", info.MaxRings), { -1.0f - margin, 0.0f });

				r2.Pop();
			}
		}
		r2.Pop();

	}

}