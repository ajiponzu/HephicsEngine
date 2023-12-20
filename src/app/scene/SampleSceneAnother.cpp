#include "../SampleApp.hpp"

void SampleSceneAnother::Initialize(const std::shared_ptr<hephics::window::Window>& ptr_window)
{
	m_windowTitle = ptr_window->GetWindowTitle();

	ptr_window->SetCallback(
		[&](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			switch (action)
			{
			case GLFW_PRESS:
				switch (key)
				{
				case GLFW_KEY_ESCAPE:
					m_isContinuous = false;
					::glfwSetWindowShouldClose(window, GLFW_TRUE);
					break;
				case GLFW_KEY_ENTER:
					m_isChangedScene = true;
					m_nextSceneName = "first";
					const auto& gpu_instance = hephics::GPUHandler::GetInstance();
					Scene::ResetScene(gpu_instance);
					break;
				}
			}
		});

	m_actors.emplace_back(std::make_shared<SampleActorAnother>());

	Scene::Initialize(ptr_window);
}

void SampleSceneAnother::Update()
{
	Scene::Update();
}

void SampleSceneAnother::Render()
{
	Scene::Render();
}