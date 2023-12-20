#include "../Interface.hpp"

void vk_interface::component::Image::SetImage(const vk::UniqueDevice& logical_device,
	const vk::ImageCreateInfo& create_info)
{
	m_image = logical_device->createImageUnique(create_info);
}

void vk_interface::component::Image::SetMemory(const vk::UniqueDevice& logical_device,
	const vk::MemoryAllocateInfo& allocate_info)
{
	m_memory = logical_device->allocateMemoryUnique(allocate_info);
}

void vk_interface::component::Image::BindMemory(const vk::UniqueDevice& logical_device)
{
	logical_device->bindImageMemory(m_image.get(), m_memory.get(), 0);
}

void vk_interface::component::Image::SetImageView(const vk::UniqueDevice& logical_device,
	const vk::ImageViewCreateInfo& create_info)
{
	vk::ImageViewCreateInfo new_create_info = create_info;
	new_create_info.setImage(m_image.get());
	m_view = logical_device->createImageViewUnique(new_create_info);
}

void vk_interface::component::Image::Clear(const vk::UniqueDevice& logical_device)
{
	logical_device->destroyImageView(m_view.get());
	m_view.release();

	logical_device->freeMemory(m_memory.get());
	m_memory.release();

	logical_device->destroyImage(m_image.get());
	m_image.release();
}