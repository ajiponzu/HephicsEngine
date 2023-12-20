#include "../Interface.hpp"

void vk_interface::component::DescriptorSet::SetDescriptorSetLayout(const vk::UniqueDevice& logical_device,
	const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
	vk::DescriptorSetLayoutCreateInfo layout_create_info({}, bindings);
	m_descriptorSetLayout = logical_device->createDescriptorSetLayoutUnique(layout_create_info, nullptr);
}

void vk_interface::component::DescriptorSet::SetDescriptorPool(const vk::UniqueDevice& logical_device,
	const vk::DescriptorPoolCreateInfo& create_info)
{
	m_descriptorPool = logical_device->createDescriptorPoolUnique(create_info);
}

void vk_interface::component::DescriptorSet::SetDescriptorSet(const vk::UniqueDevice& logical_device, const uint8_t& concurrent_frame_num)
{
	std::vector<vk::DescriptorSetLayout> desc_set_layouts(concurrent_frame_num, m_descriptorSetLayout.get());
	vk::DescriptorSetAllocateInfo alloc_info(m_descriptorPool.get(), desc_set_layouts);
	m_descriptorSets = logical_device->allocateDescriptorSetsUnique(alloc_info);
}

void vk_interface::component::DescriptorSet::UpdateDescriptorSet(const vk::UniqueDevice& logical_device,
	const size_t& target_idx, std::vector<vk::WriteDescriptorSet>&& write_descriptor_sets)
{
	for (auto& write_descriptor : write_descriptor_sets)
		write_descriptor.setDstSet(m_descriptorSets.at(target_idx).get());

	logical_device->updateDescriptorSets(write_descriptor_sets, nullptr);
}