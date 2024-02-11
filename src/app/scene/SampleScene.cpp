#include "../SampleApp.hpp"

void SampleScene::Initialize()
{
	const auto& window = hephics::window::Manager::GetWindow();

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
					break;
				case GLFW_KEY_ENTER:
					m_isChangedScene = true;
					m_nextSceneName = "second";
					break;
				}
			}
		});
	m_actors.emplace_back(std::make_shared<SampleActor>());

	Scene::Initialize();
}

void SampleScene::Update()
{
	Scene::Update();
}

void SampleScene::Render()
{
	Scene::Render();
}