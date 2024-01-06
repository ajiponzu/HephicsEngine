#include "../SampleApp.hpp"

static bool check_press_key(const std::shared_ptr<hephics::window::Window>& ptr_window, const int& key)
{
	const auto glfw_window = ptr_window->GetPtrWindow();
	return ::glfwGetKey(glfw_window, key) == GLFW_PRESS;
}

void MoveComponent::Initialize(const std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
}

void MoveComponent::Update(hephics::actor::Actor* const owner, const std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	static constexpr auto VERTICAL_MOVEMENT = 2.0f;
	static constexpr auto HORIZONTAL_MOVEMENT = 2.0f;
	static constexpr auto DEPTH_MOVEMENT = 2.0f;

	const auto& cur_window_title = gpu_instance->GetWindowTitle();
	const auto& window = hephics::window::WindowManager::GetWindow(cur_window_title);

	auto& owner_position = owner->GetPosition();
	if (check_press_key(window, GLFW_KEY_A))
	{
		glm::mat4 translation_matrix =
			glm::translate(glm::mat4(1.0f), glm::vec3(-HORIZONTAL_MOVEMENT, 0.0f, 0.0f));
		owner_position->model = translation_matrix * owner_position->model;
#ifdef _DEBUG
		std::cout << "input_key: 'A'" << std::endl;
#endif
	}
	else if (check_press_key(window, GLFW_KEY_D))
	{
		glm::mat4 translation_matrix =
			glm::translate(glm::mat4(1.0f), glm::vec3(HORIZONTAL_MOVEMENT, 0.0f, 0.0f));
		owner_position->model = translation_matrix * owner_position->model;
#ifdef _DEBUG
		std::cout << "input_key: 'D'" << std::endl;
#endif
	}
	else if (check_press_key(window, GLFW_KEY_W))
	{
		glm::mat4 translation_matrix =
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, DEPTH_MOVEMENT));
		owner_position->model = translation_matrix * owner_position->model;
#ifdef _DEBUG
		std::cout << "input_key: 'W'" << std::endl;
#endif
	}
	else if (check_press_key(window, GLFW_KEY_S))
	{
		glm::mat4 translation_matrix =
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -DEPTH_MOVEMENT));
		owner_position->model = translation_matrix * owner_position->model;
#ifdef _DEBUG
		std::cout << "input_key: 'S'" << std::endl;
#endif
	}
	else if (check_press_key(window, GLFW_KEY_K))
	{
		glm::mat4 translation_matrix =
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, HORIZONTAL_MOVEMENT, 0.0f));
		owner_position->model = translation_matrix * owner_position->model;
#ifdef _DEBUG
		std::cout << "input_key: 'K'" << std::endl;
#endif
	}
	else if (check_press_key(window, GLFW_KEY_J))
	{
		glm::mat4 translation_matrix =
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -HORIZONTAL_MOVEMENT, 0.0f));
		owner_position->model = translation_matrix * owner_position->model;
#ifdef _DEBUG
		std::cout << "input_key: 'J'" << std::endl;
#endif
	}
}

void MoveComponent::Render(const std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
}