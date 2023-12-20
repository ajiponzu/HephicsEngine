#include "../Hephics.hpp"

std::unordered_map<std::string, size_t> hephics::GPUHandler::s_graphicPurposeDictionary;
std::shared_ptr<hephics::VkInstance> hephics::GPUHandler::s_ptrGPUInstance;

void hephics::GPUHandler::AddPurpose(const std::vector<std::string>& purpose_list)
{
	size_t count = 0;
	for (const auto& purpose : purpose_list)
	{
		s_graphicPurposeDictionary.emplace(purpose, count);
		count++;
	}
}

void hephics::GPUHandler::AddInstance(const std::shared_ptr<window::Window>& window)
{
	s_ptrGPUInstance = std::make_shared<VkInstance>(window);
}

const size_t& hephics::GPUHandler::GetPurposeIdx(const std::string& purpose)
{
	if (!s_graphicPurposeDictionary.contains(purpose))
		throw std::runtime_error("purpose: not found");

	return s_graphicPurposeDictionary.at(purpose);
}

const size_t hephics::GPUHandler::GetPurposeNumber()
{
	return s_graphicPurposeDictionary.size();
}