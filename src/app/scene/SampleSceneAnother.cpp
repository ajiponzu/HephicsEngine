#include "../SampleApp.hpp"

void SampleSceneAnother::Initialize()
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
					m_nextSceneName = "first";
					break;
				case GLFW_KEY_SPACE:
					WriteScreenImage();
					break;
				}
			}
		});

	m_actors.emplace_back(std::make_shared<SampleActorAnother>());

	Scene::Initialize();
}

void SampleSceneAnother::Update()
{
	Scene::Update();
}

void SampleSceneAnother::Render()
{
	Scene::Render();
}