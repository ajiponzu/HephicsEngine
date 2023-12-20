#include "../Interface.hpp"

void vk_interface::graphic::Pipeline::SetPipeline(const vk::UniqueDevice& logical_device,
	const vk::GraphicsPipelineCreateInfo& create_info)
{
	auto create_option = logical_device->createGraphicsPipelineUnique({}, create_info);
	if (create_option.result == vk::Result::eSuccess)
		m_pipeline = std::move(create_option.value);
	else
		throw std::runtime_error("failed to create a pipeline!");
}

void vk_interface::compute::Pipeline::SetPipeline(const vk::UniqueDevice& logical_device,
	const vk::ComputePipelineCreateInfo& create_info)
{
	auto create_option = logical_device->createComputePipelineUnique({}, create_info);
	if (create_option.result == vk::Result::eSuccess)
		m_pipeline = std::move(create_option.value);
	else
		throw std::runtime_error("failed to create a pipeline!");
}

void vk_interface::ray_tracing::Pipeline::SetPipeline(const vk::UniqueDevice& logical_device,
	const vk::RayTracingPipelineCreateInfoKHR& create_info)
{
	auto create_option = logical_device->createRayTracingPipelineKHRUnique({}, {}, create_info);
	if (create_option.result == vk::Result::eSuccess)
		m_pipeline = std::move(create_option.value);
	else
		throw std::runtime_error("failed to create a pipeline!");
}