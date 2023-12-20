#include "SampleApp.hpp"

void SampleApp::Initialize()
{
	std::ios::sync_with_stdio(false);
	std::cin.tie(nullptr);

	hephics::window::WindowManager::Initialize();
	hephics::window::WindowManager::AddWindow({ 800, 600, "main" });
	const auto window_option = hephics::window::WindowManager::GetWindow("main");
	if (!window_option.has_value())
		throw std::runtime_error("failed to create window");

	const auto& ptr_window = window_option.value();

	hephics::GPUHandler::AddInstance(ptr_window);

	m_sceneDictionary.emplace("first", [] { return std::make_shared<SampleScene>("first"); });
	m_sceneDictionary.emplace("second", [] { return std::make_shared<SampleSceneAnother>("second"); });

	m_ptrCurrentScene = m_sceneDictionary.at("first")();
	m_ptrCurrentScene->Initialize(ptr_window);
}

void SampleApp::Run()
{
	const auto window_option = hephics::window::WindowManager::GetWindow("main");
	if (!window_option.has_value())
		throw std::runtime_error("failed to create window");
	const auto& ptr_window = window_option.value();

	while (m_ptrCurrentScene->IsContinuous() &&
		!(::glfwWindowShouldClose(ptr_window->GetPtrWindow())))
	{
		::glfwPollEvents();
		if (m_ptrCurrentScene->IsChangedScene())
		{
			const auto& next_scene_name = m_ptrCurrentScene->GetNextSceneName();
			if (m_sceneDictionary.contains(next_scene_name))
			{
				const auto window_option = hephics::window::WindowManager::GetWindow(m_ptrCurrentScene->GetWindowTitle());
				if (!window_option.has_value())
					throw std::runtime_error("failed to create window");

				const auto& ptr_window = window_option.value();
				m_ptrCurrentScene = m_sceneDictionary.at(next_scene_name)();
				m_ptrCurrentScene->Initialize(ptr_window);
			}
		}

		m_ptrCurrentScene->Update();
		m_ptrCurrentScene->Render();
	}
}