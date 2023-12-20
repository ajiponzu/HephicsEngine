#include "../Hephics.hpp"

std::unordered_map<const ::GLFWwindow*,
	std::unordered_map<std::string, hephics::window::CallbackVariant>> hephics::window::Window::s_callbackDictionary;

std::unordered_map<const ::GLFWwindow*, std::string> hephics::window::Window::s_windowTitleDictionary;

std::unordered_map<std::string, std::shared_ptr<hephics::window::Window>> hephics::window::WindowManager::s_windowDictionary;

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
	s_windowTitleDictionary.emplace(m_ptrWindow, GetWindowTitle());
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

const std::string& hephics::window::Window::GetWindowTitleFromDictionary(const::GLFWwindow* const ptr_window)
{
	if (!s_windowTitleDictionary.contains(ptr_window))
		throw std::runtime_error("ptr_window: not found");

	return s_windowTitleDictionary.at(ptr_window);
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

void hephics::window::WindowManager::Initialize()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void hephics::window::WindowManager::Shutdown()
{
	for (auto& [_, ptr_window] : s_windowDictionary)
	{
		auto ptr_glfw_window = ptr_window->GetPtrWindow();
		if (ptr_glfw_window != nullptr)
			::glfwDestroyWindow(ptr_glfw_window);
	}
	::glfwTerminate();
}

void hephics::window::WindowManager::AddWindow(const WindowInfo& info)
{
	s_windowDictionary.emplace(info.title, std::make_shared<Window>(info));
}

const std::shared_ptr<hephics::window::Window>&
hephics::window::WindowManager::GetWindow(const std::string& window_key)
{
	if (!s_windowDictionary.contains(window_key))
		throw std::runtime_error("window: not found");

	return s_windowDictionary.at(window_key);
}