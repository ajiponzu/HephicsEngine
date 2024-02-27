#include "../Hephics.hpp"

std::unordered_set<std::string> hephics::GPUHandler::s_graphicPurposeSet;
std::unordered_set<std::string> hephics::GPUHandler::s_computePurposeSet;
std::shared_ptr<hephics::VkInstance> hephics::GPUHandler::s_ptrGPUInstance;

void hephics::GPUHandler::AddGraphicPurpose(const std::vector<std::string>& purpose_list)
{
	for (const auto& purpose : purpose_list)
		s_graphicPurposeSet.emplace(purpose);
}

void hephics::GPUHandler::AddComputePurpose(const std::vector<std::string>& purpose_list)
{
	for (const auto& purpose : purpose_list)
		s_computePurposeSet.emplace(purpose);
}

void hephics::GPUHandler::InitializeInstance()
{
	s_ptrGPUInstance = std::make_shared<VkInstance>();
}