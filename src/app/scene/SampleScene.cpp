#include "../SampleApp.hpp"

void SampleScene::Initialize(const std::shared_ptr<hephics::window::Window>& ptr_window)
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
					m_nextSceneName = "second";

					const auto gpu_instance_option = hephics::GPUHandler::GetInstance(m_windowTitle);
					if (!gpu_instance_option.has_value())
						throw std::runtime_error("failed to find vulkan_instance");
					auto& gpu_instance = gpu_instance_option.value();
					Scene::ResetScene(gpu_instance);
					break;
				}
			}
		});
	m_actors.emplace_back(std::make_shared<SampleActor>());

	Scene::Initialize(ptr_window);
}

void SampleScene::Update()
{
	Scene::Update();
}

void SampleScene::Render()
{
	Scene::Render();
}