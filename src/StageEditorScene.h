#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <sstream>
#include <string_view>

#include "Scene.h"
#include "Stage.h"
#include "Ref.h"
#include "EventEmitter.h"
#include "Common.h"

/*
TODO:
- Add double click to select stage from list
- Add some sort of notification/messages for when the user saves, or maybe for errors
- Go back from stage editor
- Adjust subscriptions
*/

namespace bsf
{
	class Application;
	class Renderer2D;
	class Texture2D;

	class UIPanel;
	class UIElement;
	class UIPanel;
	class UIRoot;
	class UILayer;
	
	enum class StageEditorTool
	{
		BlueSphere,
		RedSphere,
		YellowSphere,
		Bumper,
		Ring,
		AvoidSearch,
		Position
	};

	enum class UILayout
	{
		Horizontal, Vertical, Fill, Free
	};



	struct UIPalette
	{
		glm::vec4 Foreground = Colors::White;

		glm::vec4 Background = ToColor(0xff121212);
		glm::vec4 BackgroundVariant = Lighten(Background, 0.25f);

		glm::vec4 Primary = Colors::Blue;
		glm::vec4 PrimaryContrast = Colors::White;

		glm::vec4 Secondary = Colors::Yellow;
		glm::vec4 SecondaryContrast = Colors::Black;

	};

	template<typename T>
	struct UIBoundValue
	{
	public:
		UIBoundValue() = default;
		UIBoundValue(const T& def) : m_Default(def) {}
		std::function<void(const T&)> Set = [&](const T& val) { m_Default = val; };
		std::function<T(void)> Get = [&] { return m_Default; };
		std::function<std::string(void)> Format = nullptr;

		explicit operator T() { return Get(); }
		void operator=(const T& val) { Set(val); }

	private:
		T m_Default;
	};

	struct UIStyle
	{

		UIStyle() { Recompute(); }

		// Sizes
		float GlobalScale = 20.0f;
		
		float MarginUnit = -1.0f;
		
		float ShadowOffset = -1.0f;

		float StageAreaCrosshairSize = 0.3f;
		float StageAreaCrosshairThickness = 0.1f;
		float StageAreaAvoidSearchThickness = 0.3f;

		float TextShadowOffset = 0.025f;
		float LabelFontScale = 0.5f;

		float SliderTrackThickness = -1.0f;

		// Colors
		glm::vec4 ShadowColor = { 0.0f, 0.0f, 0.0f, 0.5f };
		glm::vec4 IconButtonDefaultTint = { 0.5f, 0.5f, 0.5f, 1.0f };

		UIPalette Palette;

		const glm::vec4& DefaultColor(const std::optional<glm::vec4>& colorOpt, const glm::vec4& default) const;

		const glm::vec4 GetBackgroundColor(const UIElement& element, const glm::vec4& default) const;
		const glm::vec4 GetForegroundColor(const UIElement& element, const glm::vec4& default) const;

		float GetMargin(float units = 1.0f) const { return units * MarginUnit; }
		void Recompute();
	};


	enum class UIElementFlags : uint32_t
	{
		None = 0x0,
		ReceiveHover = 0x1,
		ReceiveFocus = 0x2
	};

	class UIElement : public EventReceiver
	{
	public:
		UIElement();
		UIElement(std::underlying_type_t<UIElementFlags> flags);
		UIElement(const UIElement&) = delete;
		UIElement(UIElement&&) = delete;
		virtual ~UIElement() {}

		Rect Bounds;
		float Margin = 0.0f;
		glm::vec2 Position = { 0.0f, 0.0f };
		glm::vec2 PreferredSize = { 0.0f, 0.0f };
		glm::vec2 MaxSize = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		glm::vec2 MinSize = { 0.0f, 0.0f }; // TODO Maybe move as protected, don't want to mess around with it
		bool Hovered = false;
		bool Focused = false;

		std::optional<glm::vec4> BackgroundColor = std::nullopt;
		std::optional<glm::vec4> ForegroundColor = std::nullopt;

		EventEmitter<KeyPressedEvent> KeyPressed;
		EventEmitter<KeyReleasedEvent> KeyReleased;
		EventEmitter<CharacterTypedEvent> CharacterTyped;
		EventEmitter<MouseEvent> MouseDragged, MouseMoved, MouseClicked, MouseDblClicked, MousePressed, MouseReleased;
		EventEmitter<MouseEvent> GlobalMouseReleased;
		EventEmitter<WheelEvent> Wheel;

		virtual void Update(const UIRoot& root, const Time& time) {}
		virtual void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) = 0;
		virtual void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) = 0;
		void RenderDebugInfo(const UIRoot& root, Renderer2D& renderer, const Time& time);

		uint32_t GetId() const { return m_Id; }

		bool GetFlag(UIElementFlags flag) const;

		virtual const std::vector<Ref<UIElement>>& Children();

		void Traverse(const std::function<void(UIElement&)>& action);

		const UIStyle& GetStyle() const { return *m_Style; }

	private:
		friend class UIRoot;
		static uint32_t m_NextId;
		
		std::underlying_type_t<UIElementFlags> m_Flags;
		UIStyle* m_Style = nullptr;
		uint32_t m_Id;
	};


	class UIRoot : public EventReceiver
	{
	public:

		void SetStyle(const UIStyle& style);
		//const UIStyle& GetStyle() const { return m_Style; }

		void Attach(Application& app);
		void Detach(Application& app);

		void Render(const glm::vec2& windowSize, const glm::vec2& viewport, Renderer2D& renderer, const Time& time);
		void PushLayer(const Ref<UILayer>& layer) { m_LayersToPush.push_back(layer); }
		void PopLayer() { m_LayersToPop++; }


		glm::vec2 GetMousePosition() const { return m_MouseState.Position; }

	private:

		static constexpr auto s_ClickDelay = std::chrono::milliseconds(250);

		UIStyle m_Style;

		glm::vec2 m_Viewport, m_WindowSize;
		glm::mat4 m_Projection, m_InverseProjection;
		std::vector<Ref<UILayer>> m_Layers, m_LayersToPush;
		uint32_t m_LayersToPop = 0;

		struct MouseButtonState
		{
			MouseButtonState() = default;
			MouseButtonState(MouseButtonState&) = delete;
			MouseButtonState(MouseButtonState&&) = delete;
			bool Pressed = false;
			std::chrono::system_clock::time_point TimePressed;
			std::chrono::system_clock::time_point TimeLastClicked;
		};
		
		struct {
			Ref<UIElement> HoverTarget = nullptr;
			Ref<UIElement> DragTarget = nullptr;
			MouseButton DragButton = MouseButton::None;
			glm::vec2 Position = { 0.0f, 0.0f }, PrevPosition = { 0.0f, 0.0f };
		} m_MouseState;

		Ref<UIElement> m_FocusedControl;

		std::unordered_map<MouseButton, MouseButtonState> m_MouseButtonState;

		glm::vec2 GetMousePosition(const glm::vec2& windowMousePos) const;
		Ref<UIElement> Intersect(const Ref<UIElement>& root, const glm::vec2& pos, const std::function<bool(const UIElement&)>& predicate = nullptr);

	};

	class UIIconButton : public UIElement
	{
	public:
		UIIconButton(): UIElement(MakeFlags(UIElementFlags::ReceiveHover)) {}
		Ref<Texture2D> Icon = nullptr;
		glm::vec4 Tint = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool Selected = false;

		void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) override;
		void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) override;

	};

	class UIPanel : public UIElement
	{
	public:

		UIPanel() = default;
		UILayout Layout = UILayout::Vertical;
		
		bool HasShadow = false;

		void AddChild(const Ref<UIElement>& child);
		void RemoveChild(const Ref<UIElement>& child);

		void Update(const UIRoot& root, const Time& time) override;
		void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) override;
		void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) override;

		void Pack() { m_ShouldPack = true; }

		const std::vector<Ref<UIElement>>& Children() override { return m_Children; }

	private:
		bool m_ShouldPack = false;
		std::vector<Ref<UIElement>> m_Children;
	};

	class UILayer : public UIPanel
	{
	public:
		UILayer();
	};


	class UIStageList : public UIElement
	{
	public:

		struct StageSelectedEvent
		{
			std::string FileName;
		};

		struct StageReorderEvent
		{
			std::vector<std::string> Files;
		};


		EventEmitter<StageSelectedEvent> FileSelected;
		EventEmitter<StageReorderEvent> FileReorder;

		float ItemWidth = 2.0f;
		float ItemHeight = 2.0f;

		UIStageList(uint32_t rows, uint32_t cols);

		void SetFiles(const std::vector<std::string>& files);
		void Update(const UIRoot& root, const Time& time) override;
		void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) override;
		void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) override;

		void SetScroll(float scroll);
		float GetScroll() const;

	private:

		struct StageInfo
		{
			bool IsDragged = false;
			bool Initialized = false;
			bool Loaded = false;
			bool Visible = false;
			std::string FileName;
			std::string Name;
			uint32_t MaxRings = 0, BlueSpheres = 0;
			Ref<Texture2D> Pattern = nullptr;
			Rect TargetBounds, CurrentBounds;
			bool operator==(const StageInfo& other) { return FileName == other.FileName; }
			bool operator!=(const StageInfo& other) { return !(*this == other); }
		};

		uint32_t m_Rows = 0;
		uint32_t m_Columns = 0;
		
		float m_MaxHeightOffset = 0.0f;
		float m_HeightOffset = 0.0f;
		float m_TotalHeight = 0.0f;

		uint32_t m_TotalRows = 0;
		uint32_t m_MaxTopRow = 0;

		std::vector<std::string> m_Files;
		std::vector<StageInfo> m_StagesInfo;

		std::optional<StageInfo> m_DraggedItem;

		void RenderItem(Renderer2D& r2, const StageInfo& item);

	};


	// TODO Fix zoom relative to center or mouse position
	class UIStageEditorArea : public UIElement
	{
	public:

		StageEditorTool ActiveTool = StageEditorTool::BlueSphere;

		UIStageEditorArea();

		void Update(const UIRoot& root, const Time& time) override;
		void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) override;
		void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) override;

		void SetStage(const Ref<Stage>& stage) { m_Stage = stage; }
		void UpdatePattern();

	private:

		void DrawCursor(Renderer2D& r2, const UIStyle& style);

		glm::vec2 WorldToScreen(const glm::vec2 pos) const;
		glm::vec2 ScreenToWorld(const glm::vec2 pos) const;
		std::optional<glm::ivec2> ScreenToStage(const glm::vec2 screenPos) const;

		// StageObject: { Texture, Tint }
		std::unordered_map<EStageObject, std::tuple<Ref<Texture2D>, glm::vec4>> m_StageObjRendering;
		// Avoid search { Texture, Tint }
		std::tuple<Ref<Texture2D>, glm::vec4> m_AvoidSearchRendering;
		std::tuple<Ref<Texture2D>, glm::vec4> m_PositionRendering;

		float m_MinZoom = 0.25f, m_MaxZoom = 2.0f;

		glm::vec2 m_ViewOrigin = { 0.0f, 0.0f };
		float m_Zoom = 1.0f, m_TargetZoom = 1.0f, m_ZoomStep = 1.25f;

		std::optional<glm::ivec2> m_CursorPos;

		Ref<Stage> m_Stage = nullptr;
		Ref<Texture2D> m_Pattern = nullptr, m_BgPattern;
	};
	
	class UITextInput : public UIElement
	{
	public:
		std::string Label;

		UITextInput();

		void Bind(const UIBoundValue<std::string>& value) { m_Value = value; }

		void Update(const UIRoot& root, const Time& time) override;
		void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) override;
		void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) override;

		std::string GetValue() const { return m_Value.Get(); }
		void SetValue(const std::string& value) { m_Value.Set(value); };

	private:
		UIBoundValue<std::string> m_Value = "";
	};

	class UIText : public UIElement
	{
	public:
		float Scale = 1.0f;
		std::string Text;

		UIText() { Margin = 1.0f; }

		void Update(const UIRoot& root, const Time& time) override;
		void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) override;
		void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) override;

	};


	class UIButton : public UIElement
	{
	public:
		std::string Label = "Button";

		UIButton();

		void Update(const UIRoot& root, const Time& time) override;
		void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) override;
		void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) override;

	};
	
	enum class UISliderOrientation { Horizontal, Vertical };


	
	class UISlider : public UIElement
	{
	public:

		struct ValueChangedEvent
		{
			float Value;
		};


		EventEmitter<ValueChangedEvent> ValueChanged;

		std::string Label = "Color Picker";
		float Min = 0.0f, Max = 1.0f;
		UISliderOrientation Orientation = UISliderOrientation::Horizontal;
		bool ShowValue = false;

		UISlider();

		void Bind(const UIBoundValue<float>& value) { m_Value = value; }
		void Update(const UIRoot& root, const Time& time) override;
		void Render(const UIRoot& root, Renderer2D& renderer, const Time& time) override;
		void UpdateBounds(const UIRoot& root, const glm::vec2& origin, const glm::vec2& computedSize) override;

	private:

		Rect GetContentBounds() const;

		bool IntersectHandle(const glm::vec2& pos) const;
		float GetDelta() const;
		glm::vec2 GetHandlePosition() const;

		std::array<glm::vec2, 2> m_Limits;

		UIBoundValue<float> m_Value = 0.0f;
	};

	class UIColorPicker : public UIPanel
	{
	public:

		using BindFn = std::function<glm::vec3&()>;

		std::string Label = "Color Picker";

		void Bind(const BindFn& fn);

		UIColorPicker();

		void Update(const UIRoot& root, const Time& time);

	private:
		BindFn m_GetValue = [&]() -> glm::vec3& { return m_DefaultValue; };
		std::array<Ref<UISlider>, 3> m_RGB;
		Ref<UIText> m_Label;
		glm::vec3 m_DefaultValue = Colors::Black;
	};
	

	class StageEditorScene : public Scene
	{
	public:
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;
		
		~StageEditorScene() {}

	private:

		std::optional<std::string> m_CurrentStageFile;
		Ref<Stage> m_CurrentStage;
		Ref<UIRoot> m_uiRoot;
		Ref<UILayer> m_uiStageListDialogLayer, m_uiConfirmDialogLayer;
		Ref<UIStageEditorArea> m_uiEditorArea;
		Ref<UIStageList> m_uiStageList;
		Ref<UIText> m_uiConfirmDialogText;
		Ref<UIPanel> m_uiConfirmDialogPanel;

		std::vector<Ref<UIIconButton>> m_ToolButtons;

		std::function<void()> m_OnConfirmDialogConfirm = nullptr;

		void ShowConfirmDialog(const std::string& text, const std::function<void()>& onConfirm);

		void OnNewButtonClick(const MouseEvent& evt) { NewStage(); }
		void OnSaveButtonClick(const MouseEvent& evt);
		void OnStageListButtonClick(const MouseEvent& evt);

		void NewStage();
		void LoadStage(std::string_view fileName);
		void InitializeUI();


	};

	

}