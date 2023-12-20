#include "../Interface.hpp"

void vk_interface::component::Fence::SetFence(const vk::UniqueDevice& logical_device,
	const vk::FenceCreateInfo& create_info)
{
	m_fence = logical_device->createFenceUnique(create_info);
}

void vk_interface::component::Fence::Signal(const vk::UniqueDevice& logical_device)
{
	logical_device->resetFences({ m_fence.get() });
}

void vk_interface::component::Fence::Wait(const vk::UniqueDevice& logical_device,
	const uint64_t& timeout)
{
	vk::resultCheck(logical_device->waitForFences({ m_fence.get() }, VK_TRUE, timeout), "error: wait_for_fences");
}