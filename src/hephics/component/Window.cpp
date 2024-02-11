#include "../Hephics.hpp"

std::unordered_map<const ::GLFWwindow*,
	std::unordered_map<std::string, hephics::window::CallbackVariant>> hephics::window::Window::s_callbackDictionary;

std::shared_ptr<hephics::window::Window> hephics::window::Manager::s_ptrWindow;

glm::vec2 hephics::window::Manager::s_cursorPosition;
glm::vec2 hephics::window::Manager::s_mouseScroll;

static void input_key_callback(::GLFWwindow* ptr_window, int key, int scancode, int action, int mods)
{
	using CallbackType = hephics::window::InputKeyCallback;
	try
	{
		const auto& callback_variant = hephics::window::Window::GetCallback<CallbackType>(ptr_window);
		const auto& input_key_func = std::get<CallbackType>(callback_variant);
		input_key_func(ptr_window, key, scancode, action, mods);
	}
	catch (std::exception e)
	{
		return;
	}
}

static void cursor_position_callback(::GLFWwindow* ptr_window, double xpos, double ypos)
{
	using CallbackType = hephics::window::CursorPositionCallback;
	hephics::window::Manager::SetCursorPosition(xpos, ypos);
	try
	{
		const auto& callback_variant = hephics::window::Window::GetCallback<CallbackType>(ptr_window);
		const auto& cursor_position_func = std::get<CallbackType>(callback_variant);
		cursor_position_func(ptr_window, xpos, ypos);
	}
	catch (std::exception e)
	{
		return;
	}
}

static void mouse_button_callback(::GLFWwindow* ptr_window, int button, int action, int mods)
{
	using CallbackType = hephics::window::MouseButtonCallback;
	try
	{
		const auto& callback_variant = hephics::window::Window::GetCallback<CallbackType>(ptr_window);
		const auto& mouse_button_func = std::get<CallbackType>(callback_variant);
		mouse_button_func(ptr_window, button, action, mods);
	}
	catch (std::exception e)
	{
		return;
	}
}

static void mouse_scroll_callback(::GLFWwindow* ptr_window, double xoffset, double yoffset)
{
	using CallbackType = hephics::window::MouseScrollCallback;
	hephics::window::Manager::SetMouseScroll(xoffset, yoffset);
	std::cout << std::format("scroll: ({}, {})", xoffset, yoffset) << std::endl;
	try
	{
		const auto& callback_variant = hephics::window::Window::GetCallback<CallbackType>(ptr_window);
		const auto& mouse_scroll_func = std::get<CallbackType>(callback_variant);
		mouse_scroll_func(ptr_window, xoffset, yoffset, 0);
	}
	catch (std::exception e)
	{
		return;
	}
}

static void window_resized_callback(::GLFWwindow* ptr_window, int width, int height)
{
	using CallbackType = hephics::window::WindowResizedCallback;
	auto& gpu_instance = hephics::GPUHandler::GetInstance();
	hephics::window::Manager::SetWindowSize(width, height);
	gpu_instance->ResetSwapChain(ptr_window);

	try
	{
		const auto& callback_variant = hephics::window::Window::GetCallback<CallbackType>(ptr_window);
		const auto& window_resized_func = std::get<CallbackType>(callback_variant);
		window_resized_func(ptr_window, width, height);
	}
	catch (std::exception e)
	{
		return;
	}
}

void hephics::window::Window::SetCallbacks()
{
	::glfwSetKeyCallback(m_ptrWindow, input_key_callback);
	::glfwSetCursorPosCallback(m_ptrWindow, cursor_position_callback);
	::glfwSetMouseButtonCallback(m_ptrWindow, mouse_button_callback);
	::glfwSetScrollCallback(m_ptrWindow, mouse_scroll_callback);
	::glfwSetWindowSizeCallback(m_ptrWindow, window_resized_callback);
}

hephics::window::Window::Window(const WindowInfo& info)
	: m_info(info)
{
	m_ptrWindow =
		::glfwCreateWindow(m_info.width, m_info.height, m_info.title.c_str(), nullptr, nullptr);
	SetCallbacks();
}

void hephics::window::Window::SetCallback(InputKeyCallback&& callback) const
{
	try
	{
		const auto callback_tag = typeid(decltype(callback)).name();
		s_callbackDictionary[m_ptrWindow][callback_tag] = callback;
	}
	catch (const std::exception& exception)
	{
		std::cerr << std::format("SetCallback: {}\n", exception.what());
	}
}

void hephics::window::Window::SetCallback(CursorPositionCallback&& callback) const
{
	try
	{
		const auto callback_tag = typeid(decltype(callback)).name();
		s_callbackDictionary[m_ptrWindow][callback_tag] = callback;
	}
	catch (const std::exception& exception)
	{
		std::cerr << std::format("SetCallback: {}\n", exception.what());
	}
}

void hephics::window::Window::SetCallback(MouseButtonCallback&& callback) const
{
	try
	{
		const auto callback_tag = typeid(decltype(callback)).name();
		s_callbackDictionary[m_ptrWindow][callback_tag] = callback;
	}
	catch (const std::exception& exception)
	{
		std::cerr << std::format("SetCallback: {}\n", exception.what());
	}
}

void hephics::window::Window::SetCallback(MouseScrollCallback&& callback) const
{
	try
	{
		const auto callback_tag = typeid(decltype(callback)).name();
		s_callbackDictionary[m_ptrWindow][callback_tag] = callback;
	}
	catch (const std::exception& exception)
	{
		std::cerr << std::format("SetCallback: {}\n", exception.what());
	}
}

void hephics::window::Window::SetCallback(WindowResizedCallback&& callback) const
{
	try
	{
		const auto callback_tag = typeid(decltype(callback)).name();
		s_callbackDictionary[m_ptrWindow][callback_tag] = callback;
	}
	catch (const std::exception& exception)
	{
		std::cerr << std::format("SetCallback: {}\n", exception.what());
	}
}

template<typename T>
hephics::window::CallbackVariant hephics::window::Window::GetCallback(const ::GLFWwindow* const ptr_window)
{
	if (!s_callbackDictionary.contains(ptr_window))
		throw std::runtime_error("window_callback: not found");

	const auto& callback_dictionary = s_callbackDictionary.at(ptr_window);
	const auto callback_tag = typeid(T).name();

	if (!callback_dictionary.contains(callback_tag))
		throw std::runtime_error("window_callback: not found");

	return callback_dictionary.at(callback_tag);
}

void hephics::window::Manager::Initialize()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void hephics::window::Manager::Shutdown()
{
	auto ptr_glfw_window = s_ptrWindow->GetPtrWindow();
	if (ptr_glfw_window != nullptr)
		::glfwDestroyWindow(ptr_glfw_window);
	::glfwTerminate();
}

void hephics::window::Manager::InitializeWindow(const WindowInfo& info)
{
	s_ptrWindow = std::make_shared<Window>(info);
}

const std::shared_ptr<hephics::window::Window>&
hephics::window::Manager::GetWindow()
{
	return s_ptrWindow;
}

const glm::vec2& hephics::window::Manager::GetCursorPosition()
{
	return s_cursorPosition;
}

const glm::vec2& hephics::window::Manager::GetMouseScroll()
{
	return s_mouseScroll;
}

const bool hephics::window::Manager::CheckPressKey(const int32_t& key)
{
	const auto glfw_window = s_ptrWindow->GetPtrWindow();
	return ::glfwGetKey(glfw_window, key) == GLFW_PRESS;
}

void hephics::window::Manager::SetCursorPosition(const double& pos_x, const double& pos_y)
{
	s_cursorPosition[0] = static_cast<float_t>(pos_x);
	s_cursorPosition[1] = static_cast<float_t>(pos_y);
}

void hephics::window::Manager::SetMouseScroll(const double& x_offset, const double& y_offset)
{
	s_mouseScroll[0] = static_cast<float_t>(x_offset);
	s_mouseScroll[1] = static_cast<float_t>(y_offset);
}

void hephics::window::Manager::SetWindowSize(const int32_t& width, const int32_t& height)
{
	s_ptrWindow->m_info.width = width;
	s_ptrWindow->m_info.height = height;
}