#include "../Hephics.hpp"

std::unordered_map<std::string, std::shared_ptr<hephics::VkInstance>> hephics::GPUHandler::s_ptrVkInstanceDictionary;

void hephics::GPUHandler::AddInstance(const std::shared_ptr<window::Window>& ptr_window)
{
	s_ptrVkInstanceDictionary.emplace(ptr_window->GetWindowTitle(), std::make_shared<VkInstance>(ptr_window));
}

std::optional<std::shared_ptr<hephics::VkInstance>>
hephics::GPUHandler::GetInstance(const std::string& instance_key)
{
	if (s_ptrVkInstanceDictionary.contains(instance_key))
		return s_ptrVkInstanceDictionary.at(instance_key);

	return std::nullopt;
}