#include "../Hephics.hpp"

std::unordered_set<std::string> hephics::GPUHandler::s_graphicPurposeSet;
std::shared_ptr<hephics::VkInstance> hephics::GPUHandler::s_ptrGPUInstance;

void hephics::GPUHandler::AddGraphicPurpose(const std::vector<std::string>& purpose_list)
{
	size_t count = 0;
	for (const auto& purpose : purpose_list)
	{
		s_graphicPurposeSet.emplace(purpose);
		count++;
	}
}

void hephics::GPUHandler::InitializeInstance()
{
	s_ptrGPUInstance = std::make_shared<VkInstance>();
}