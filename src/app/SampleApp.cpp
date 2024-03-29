#include "SampleApp.hpp"

void SampleApp::Initialize()
{
	std::ios::sync_with_stdio(false);
	std::cin.tie(nullptr);

	hephics::window::Manager::Initialize();
	hephics::window::Manager::InitializeWindow({ 800, 600, "main" });
	const auto& window = hephics::window::Manager::GetWindow();

	hephics::GPUHandler::AddGraphicPurpose({ "render", "copy" });
	hephics::GPUHandler::AddComputePurpose({ "particle" });
	hephics::GPUHandler::InitializeInstance();

	m_sceneDictionary.emplace("first", [] { return std::make_shared<SampleScene>("first"); });
	m_sceneDictionary.emplace("second", [] { return std::make_shared<SampleSceneAnother>("second"); });
	m_sceneDictionary.emplace("compute", [] { return std::make_shared<SampleComputeScene>("compute"); });

	m_ptrCurrentScene = m_sceneDictionary.at("first")();
	m_ptrCurrentScene->Initialize();
}

void SampleApp::Run()
{
	const auto& window = hephics::window::Manager::GetWindow();

	while (m_ptrCurrentScene->IsContinuous() &&
		!(::glfwWindowShouldClose(window->GetPtrWindow())))
	{
		::glfwPollEvents();

		if (m_ptrCurrentScene->IsChangedScene())
		{
			const auto& next_scene_name = m_ptrCurrentScene->GetNextSceneName();
			if (m_sceneDictionary.contains(next_scene_name))
			{
				hephics::Scene::ResetScene();
				m_ptrCurrentScene = m_sceneDictionary.at(next_scene_name)();
				m_ptrCurrentScene->Initialize();
			}
		}

		m_ptrCurrentScene->Update();
		m_ptrCurrentScene->Render();
	}
}