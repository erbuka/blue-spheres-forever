#pragma once

#include "Ref.h"
#include "Scene.h"
#include "EventEmitter.h"

#include <glm/glm.hpp>

struct GLFWwindow;

namespace bsf {


	class AudioDevice;
	class Renderer2D;
	class Scene;

	class Application
	{
	public:

		EventEmitter<MouseEvent> MousePressed;
		EventEmitter<MouseEvent> MouseReleased;
		EventEmitter<MouseEvent> MouseMoved;
		EventEmitter<WheelEvent> Wheel;

		EventEmitter<WindowResizedEvent> WindowResized;

		EventEmitter<KeyPressedEvent> KeyPressed;
		EventEmitter<KeyReleasedEvent> KeyReleased;

		EventEmitter<CharacterTypedEvent> CharacterTyped;

		Application();
		~Application();
		
		Application(Application&) = delete;
		Application(Application&&) = delete;

		glm::vec2 GetWindowSize() const;

		Renderer2D& GetRenderer2D();

		AudioDevice& GetAudioDevice();

		void Start();
		void GotoScene(const std::shared_ptr<Scene> scene);

		void Exit();

	private:

		void RunScheduledTasks(const Time& time, const Ref<Scene>& scene, ESceneTaskEvent evt);

		bool m_Running = false;

		Ref<Scene> m_NextScene, m_CurrentScene;
		Ref<Renderer2D> m_Renderer2D;
		Ref<AudioDevice> m_AudioDevice;
		GLFWwindow* m_Window;
	};

}