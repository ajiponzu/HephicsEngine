#include "../SampleApp.hpp"

void SampleScene::Initialize(const std::shared_ptr<hephics::window::Window>& window)
{
	window->SetCallback(
		[&](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			switch (action)
			{
			case GLFW_PRESS:
				switch (key)
				{
				case GLFW_KEY_ESCAPE:
					m_isContinuous = false;
					Scene::ResetScene();
					::glfwSetWindowShouldClose(window, GLFW_TRUE);
					break;
				case GLFW_KEY_ENTER:
					m_isChangedScene = true;
					m_nextSceneName = "second";
					Scene::ResetScene();
					break;
				}
			}
		});
	m_actors.emplace_back(std::make_shared<SampleActor>());

	Scene::Initialize(window);
}

void SampleScene::Update()
{
	Scene::Update();
}

void SampleScene::Render()
{
	Scene::Render();
}