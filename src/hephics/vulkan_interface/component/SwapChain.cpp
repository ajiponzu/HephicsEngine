#include "../Interface.hpp"

void vk_interface::component::SwapChain::SetSwapChain(const vk::UniqueDevice& logical_device,
	const vk::SwapchainCreateInfoKHR& create_info)
{
	m_swapChain = logical_device->createSwapchainKHRUnique(create_info);
	m_images = logical_device->getSwapchainImagesKHR(m_swapChain.get());
	m_extent = create_info.imageExtent;
	m_imageFormat = create_info.imageFormat;
}

void vk_interface::component::SwapChain::SetImageViews(const vk::UniqueDevice& logical_device,
	const vk::ImageViewCreateInfo& create_info)
{
	for (const auto& vk_image : m_images)
	{
		vk::ImageViewCreateInfo new_create_info = create_info;
		new_create_info.setImage(vk_image);
		new_create_info.setFormat(m_imageFormat);
		m_imageViews.emplace_back(logical_device->createImageViewUnique(new_create_info));
	}
}

void vk_interface::component::SwapChain::SetRenderPass(const vk::UniqueDevice& logical_device,
	const vk::RenderPassCreateInfo& create_info)
{
	m_renderPass = logical_device->createRenderPassUnique(create_info);
}

void vk_interface::component::SwapChain::SetFramebuffers(const vk::UniqueDevice& logical_device, const vk::FramebufferCreateInfo& create_info)
{
	for (const auto& image_view : m_imageViews)
	{
		vk::FramebufferCreateInfo new_create_info = create_info;
		auto attachments = std::vector{ m_ptrColorImage->GetView().get(), m_ptrDepthImage->GetView().get(), image_view.get() };
		new_create_info.setAttachments(attachments);
		m_framebuffers.emplace_back(logical_device->createFramebufferUnique(new_create_info));
	}
}

void vk_interface::component::SwapChain::SetSyncObjects(const vk::UniqueDevice& logical_device, const int32_t& buffering_num)
{
	m_bufferingNum = buffering_num;

	for (uint32_t buffering_id = 0U; buffering_id < m_bufferingNum; buffering_id++)
	{
		m_imageAvailableSemaphores.emplace_back(logical_device->createSemaphoreUnique({}));
		m_finishedSemaphores.emplace_back(logical_device->createSemaphoreUnique({}));

		Fence fence;
		fence.SetFence(logical_device, { vk::FenceCreateFlagBits::eSignaled });
		m_swapFences.emplace_back(std::make_shared<Fence>(std::move(fence)));
	}

	m_tempFences.resize(m_framebuffers.size());
}

void vk_interface::component::SwapChain::AcquireNextImageIdx(const vk::UniqueDevice& logical_device)
{
	m_swapFences.at(m_currentFrameId)->Wait(logical_device, UINT64_MAX);

	const auto next_image_result = logical_device->acquireNextImageKHR(m_swapChain.get(),
		UINT64_MAX, m_imageAvailableSemaphores.at(m_currentFrameId).get());
	vk::resultCheck(next_image_result.result, "failed to acquire next image");

	m_nextImageId = next_image_result.value;
}

void vk_interface::component::SwapChain::WaitFence(const vk::UniqueDevice& logical_device)
{
	const auto& temp_fence = m_tempFences.at(m_nextImageId);
	if (temp_fence)
		vk::resultCheck(logical_device->waitForFences(m_tempFences.at(m_nextImageId), VK_TRUE, UINT64_MAX), "wait_for_fences");
	m_tempFences.at(m_nextImageId) = m_swapFences.at(m_currentFrameId)->GetFence().get();
}

void vk_interface::component::SwapChain::CancelWaitFence(const vk::UniqueDevice& logical_device)
{
	m_swapFences.at(m_currentFrameId)->Signal(logical_device);
}

void vk_interface::component::SwapChain::PrepareNextFrame()
{
	m_currentFrameId = (m_currentFrameId + 1) % m_bufferingNum;
}

void vk_interface::component::SwapChain::Clear(const vk::UniqueDevice& logical_device)
{
	m_ptrDepthImage->Clear(logical_device);
	m_ptrColorImage->Clear(logical_device);

	for (auto& framebuffer : m_framebuffers)
	{
		logical_device->destroyFramebuffer(framebuffer.get());
		framebuffer.release();
	}
	for (auto& image_view : m_imageViews)
	{
		logical_device->destroyImageView(image_view.get());
		image_view.release();
	}
	for (auto& semaphore : m_finishedSemaphores)
	{
		logical_device->destroySemaphore(semaphore.get());
		semaphore.release();
	}
	for (auto& semaphore : m_imageAvailableSemaphores)
	{
		logical_device->destroySemaphore(semaphore.get());
		semaphore.release();
	}

	m_tempFences.clear();
	m_imageViews.clear();
	m_finishedSemaphores.clear();
	m_imageAvailableSemaphores.clear();
	m_framebuffers.clear();
	m_swapFences.clear();

	logical_device->destroySwapchainKHR(m_swapChain.get());
	m_swapChain.release();
}

vk::RenderPassBeginInfo vk_interface::component::SwapChain::GetRenderPassBeginInfo(const std::vector<vk::ClearValue>& clear_values) const
{
	return vk::RenderPassBeginInfo(
		m_renderPass.get(), m_framebuffers.at(m_nextImageId).get(),
		vk::Rect2D{ vk::Offset2D{0, 0}, m_extent }, clear_values
	);
}

std::pair<vk::Viewport, vk::Rect2D> vk_interface::component::SwapChain::GetViewportAndScissor() const
{
	vk::Viewport viewport(
		0.0f, 0.0f,
		static_cast<float_t>(m_extent.width), static_cast<float_t>(m_extent.height),
		0.0f, 1.0f
	);

	vk::Rect2D scissor(
		{ 0, 0 }, m_extent
	);

	return { viewport, scissor };
}

vk::SubmitInfo vk_interface::component::SwapChain::GetRenderingSubmitInfo(
	const std::vector<vk::CommandBuffer>& submitted_command_buffers, const vk::PipelineStageFlags& wait_stage_flags) const
{
	return vk::SubmitInfo(
		m_imageAvailableSemaphores.at(m_currentFrameId).get(), wait_stage_flags,
		submitted_command_buffers, m_finishedSemaphores.at(m_currentFrameId).get()
	);
}

vk::PresentInfoKHR vk_interface::component::SwapChain::GetPresentInfo() const
{
	return vk::PresentInfoKHR(
		m_finishedSemaphores.at(m_currentFrameId).get(),
		m_swapChain.get(), m_nextImageId
	);
}