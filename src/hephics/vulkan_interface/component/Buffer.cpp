#include "../Interface.hpp"

void vk_interface::component::Buffer::SetBuffer(const vk::UniqueDevice& logical_device,
	const vk::BufferCreateInfo& create_info)
{
	m_buffer = logical_device->createBufferUnique(create_info);
	m_size = create_info.size;
}

void vk_interface::component::Buffer::SetMemory(const vk::UniqueDevice& logical_device,
	const vk::MemoryAllocateInfo& allocate_info)
{
	m_memory = logical_device->allocateMemoryUnique(allocate_info);
}

void vk_interface::component::Buffer::CopyBufferMemoryData(const vk::UniqueDevice& logical_device, const void* src_address)
{
	void* mapped_data = logical_device->mapMemory(m_memory.get(), 0, m_size, {});
	std::memcpy(mapped_data, src_address, static_cast<size_t>(m_size));
	logical_device->unmapMemory(m_memory.get());
}

void vk_interface::component::Buffer::Unmapping(const vk::UniqueDevice& logical_device)
{
	logical_device->unmapMemory(m_memory.get());
}

void vk_interface::component::Buffer::BindMemory(const vk::UniqueDevice& logical_device)
{
	logical_device->bindBufferMemory(m_buffer.get(), m_memory.get(), 0);
}

void* vk_interface::component::Buffer::Mapping(const vk::UniqueDevice& logical_device)
{
	return logical_device->mapMemory(m_memory.get(), 0, m_size, {});
}