#pragma once

#include "Common.h"
#include "EventEmitter.h"

#include <glm/glm.hpp>

struct GLFWwindow;

namespace bsf {


	class Scene;

	class Application
	{
	public:

		EventEmitter<MouseEvent> MousePressed;
		EventEmitter<MouseEvent> MouseReleased;
		EventEmitter<MouseEvent> MouseMoved;

		EventEmitter<WindowResizedEvent> WindowResized;

		EventEmitter<KeyPressedEvent> KeyPressed;
		EventEmitter<KeyReleasedEvent> KeyReleased;

		Application();
		~Application();
		
		Application(Application&) = delete;
		Application(Application&&) = delete;

		glm::vec2 GetWindowSize() const;

		void Start();

		void GotoScene(const std::shared_ptr<Scene> scene);

	private:
		std::shared_ptr<Scene> m_NextScene, m_CurrentScene;
		GLFWwindow* m_Window;
	};

}