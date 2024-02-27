#include "../Hephics.hpp"

void hephics::ComputingSyncObject::SetSyncObjects(const vk::UniqueDevice& logical_device, const int32_t& buffering_num)
{
	for (uint32_t buffering_id = 0U; buffering_id < BUFFERING_FRAME_NUM; buffering_id++)
	{
		m_semaphores.emplace_back(logical_device->createSemaphoreUnique({}));

		vk_interface::component::Fence fence;
		fence.SetFence(logical_device, { vk::FenceCreateFlagBits::eSignaled });
		m_fences.emplace_back(std::make_shared<decltype(fence)>(std::move(fence)));
	}
}

vk::SubmitInfo hephics::ComputingSyncObject::GetComputingSubmitInfo(
	const std::vector<vk::CommandBuffer>& submitted_command_buffers) const
{
	return vk::SubmitInfo(
		{}, {}, submitted_command_buffers, m_semaphores.at(m_currentFrameId).get()
	);
}

void hephics::ComputingSyncObject::WaitFence(const vk::UniqueDevice& logical_device)
{
	vk::resultCheck(logical_device->waitForFences(m_fences.at(m_currentFrameId)->GetFence().get(),
		VK_TRUE, UINT64_MAX), "wait_for_fences");
}

void hephics::ComputingSyncObject::CancelWaitFence(const vk::UniqueDevice& logical_device)
{
	m_fences.at(m_currentFrameId)->Signal(logical_device);
}

void hephics::ComputingSyncObject::PrepareNextFrame()
{
	m_currentFrameId = (m_currentFrameId + 1) % BUFFERING_FRAME_NUM;
}

void hephics::ComputingSyncObject::Clear(const vk::UniqueDevice& logical_device)
{
	for (auto& semaphore : m_semaphores)
	{
		logical_device->destroySemaphore(semaphore.get());
		semaphore.release();
	}
}

void hephics::VkInstance::SetInstance(const vk::ApplicationInfo& app_info)
{
	static vk::DynamicLoader dl;
	auto vk_get_instance_proc_addr = dl.getProcAddress<::PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);

#ifdef _DEBUG
	if (!hephics_helper::vk_init::check_validation_layer_support())
		throw std::runtime_error("validation layers requested, but not available!");
#endif

	const auto extensions = hephics_helper::vk_init::get_required_extensions();

#ifdef _DEBUG
	const auto validation_layers = hephics_helper::vk_init::get_validation_layers();
	const auto severity_flags = hephics_helper::simple_create_info::get_severity_flags();
	const auto message_type_flags = hephics_helper::simple_create_info::get_message_type_flags();
	vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> create_info(
		{ {}, &app_info, validation_layers, extensions },
		{ {}, severity_flags, message_type_flags, &hephics_helper::vk_init::debug_utils_messenger_callback });

	m_instance = vk::createInstanceUnique(create_info.get<vk::InstanceCreateInfo>());
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
	m_debugUtilsMessenger = m_instance->createDebugUtilsMessengerEXTUnique(
		vk::DebugUtilsMessengerCreateInfoEXT({}, severity_flags, message_type_flags,
			&hephics_helper::vk_init::debug_utils_messenger_callback));
#else
	vk::InstanceCreateInfo create_info({}, &app_info, {}, extensions);
	m_instance = vk::createInstanceUnique(create_info, nullptr);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
#endif
}

void hephics::VkInstance::SetWindowSurface()
{
	const auto& window = window::Manager::GetWindow();

	::VkSurfaceKHR surface;
	if (::glfwCreateWindowSurface(::VkInstance(m_instance.get()), window->GetPtrWindow(), nullptr, &surface)
		!= ::VkResult::VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");

	m_windowSurface = vk::UniqueSurfaceKHR(surface, { m_instance.get() });
}

void hephics::VkInstance::SetPhysicalDevice()
{
	const auto physical_devices = m_instance->enumeratePhysicalDevices();

	for (const auto& physical_device : physical_devices)
	{
		m_physicalDevice = physical_device;

		m_queueFamilyIndices = hephics_helper::vk_init::find_queue_families(m_physicalDevice, m_windowSurface);
		const auto extensions_supported = hephics_helper::vk_init::check_device_extension_support(physical_device);

		auto swap_chain_adequate = false;
		if (extensions_supported)
		{
			const auto swap_chain_support = hephics_helper::vk_init::query_swap_chain_support(m_physicalDevice, m_windowSurface);
			swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
		}

		const auto physical_device_features = physical_device.getFeatures();

		const auto is_suitable = m_queueFamilyIndices.is_complete()
			&& extensions_supported
			&& swap_chain_adequate
			&& physical_device_features.samplerAnisotropy;
		if (!is_suitable)
			throw std::runtime_error("physical_device: not suitable");

#ifdef _DEBUG
		std::cout << std::format("vulkan_device: {}\n", physical_device.getProperties().deviceName.data());
#endif
	}

	if (!m_physicalDevice)
		throw std::runtime_error("failed to find a suitable GPU!");
}

void hephics::VkInstance::SetLogicalDeviceAndQueue()
{
	std::vector<vk::DeviceQueueCreateInfo> queue_create_info_list;
	std::set<uint32_t> unique_queue_families =
	{ m_queueFamilyIndices.graphics_and_compute_family.value(), m_queueFamilyIndices.present_family.value() };

	static constexpr auto queue_priority = 1.0f;
	for (const auto& queue_family : unique_queue_families)
		queue_create_info_list.emplace_back(vk::DeviceQueueCreateInfo({}, queue_family, 1, &queue_priority));

	const auto device_extensions = hephics_helper::vk_init::get_device_extensions();
	vk::PhysicalDeviceFeatures device_features{};
	device_features.setSamplerAnisotropy(VK_TRUE);
	device_features.setFillModeNonSolid(VK_TRUE);
	device_features.setFullDrawIndexUint32(VK_TRUE);
	vk::DeviceCreateInfo create_info({}, queue_create_info_list, {}, device_extensions, &device_features);

#ifdef _DEBUG
	const auto validation_layers = hephics_helper::vk_init::get_validation_layers();
	create_info.setPEnabledLayerNames(validation_layers);
#endif

	m_logicalDevice = m_physicalDevice.createDeviceUnique(create_info);
	m_queuesDictionary.emplace(vk::QueueFlagBits::eGraphics, std::unordered_map<std::string, vk::Queue>
	{
		{ "graphics", m_logicalDevice->getQueue(m_queueFamilyIndices.graphics_and_compute_family.value(), 0)},
		{ "present", m_logicalDevice->getQueue(m_queueFamilyIndices.present_family.value(), 0) }
	});

	if (!GPUHandler::GetComputePurpose().empty())
	{
		m_ptrComputingSyncObject = std::make_shared<ComputingSyncObject>();
		m_ptrComputingSyncObject->SetSyncObjects(m_logicalDevice, hephics::BUFFERING_FRAME_NUM);

		m_queuesDictionary.emplace(vk::QueueFlagBits::eCompute, std::unordered_map<std::string, vk::Queue>
		{
			{ "compute", m_logicalDevice->getQueue(m_queueFamilyIndices.graphics_and_compute_family.value(), 0)}
		});
	}
}

void hephics::VkInstance::SetSwapChain()
{
	const auto& window = window::Manager::GetWindow();
	const auto swap_chain_support = hephics_helper::vk_init::query_swap_chain_support(m_physicalDevice, m_windowSurface);

	const auto surface_format = hephics_helper::vk_init::choose_swap_surface_format(swap_chain_support.formats,
		vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear);
	const auto present_mode = hephics_helper::vk_init::choose_swap_present_mode(swap_chain_support.present_modes,
		vk::PresentModeKHR::eMailbox);
	const auto extent = hephics_helper::vk_init::choose_swap_extent(swap_chain_support.capabilities,
		window->GetPtrWindow());

	auto image_count = swap_chain_support.capabilities.minImageCount + 1;

	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
		image_count = swap_chain_support.capabilities.maxImageCount;

	vk::SwapchainCreateInfoKHR create_info(
		{}, m_windowSurface.get(), image_count, surface_format.format, surface_format.colorSpace,
		extent, 1, vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive, {}, swap_chain_support.capabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque, present_mode, VK_TRUE, nullptr);

	if (!m_queueFamilyIndices.is_equal_families())
	{
		create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
		const auto& queue_family_array = m_queueFamilyIndices.get_families_array();
		create_info.setQueueFamilyIndices(queue_family_array);
	}

	m_ptrSwapChain->SetSwapChain(m_logicalDevice, create_info);
}

void hephics::VkInstance::SetSwapChainImageViews()
{
	m_ptrSwapChain->SetImageViews(m_logicalDevice,
		hephics_helper::simple_create_info::get_swap_chain_image_view_info());
}

void hephics::VkInstance::SetSwapChainRenderPass()
{
	const auto image_sample_count = hephics_helper::vk_init::get_multi_sample_count(m_physicalDevice);
	auto attachments =
		hephics_helper::simple_create_info::get_renderpass_attachment_descriptions(
			image_sample_count, m_ptrSwapChain->GetImageFormat(), FindDepthFormat());

	vk::AttachmentReference color_attachment_ref(0, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depth_attachment_ref(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::AttachmentReference color_resolve_attachment_ref(2, vk::ImageLayout::eColorAttachmentOptimal);
	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics,
		{}, color_attachment_ref, color_resolve_attachment_ref, &depth_attachment_ref);

	auto dependency = hephics_helper::simple_create_info::get_renderpass_dependency();
	vk::RenderPassCreateInfo create_info({}, attachments, subpass, dependency);
	m_ptrSwapChain->SetRenderPass(m_logicalDevice, create_info);
}

void hephics::VkInstance::SetSwapChainFramebuffers()
{
	const auto color_format = m_ptrSwapChain->GetImageFormat();
	const auto depth_format = FindDepthFormat();
	const auto& queue_family_array = m_queueFamilyIndices.get_families_array();
	const auto image_sample_count = hephics_helper::vk_init::get_multi_sample_count(m_physicalDevice);

	{
		auto& swap_chain_color_image = m_ptrSwapChain->GetColorImage();

		vk::ImageCreateInfo image_create_info({}, vk::ImageType::e2D, color_format,
			vk::Extent3D(m_ptrSwapChain->GetExtent2D(), 1U), 1, 1, image_sample_count, vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive, queue_family_array);
		swap_chain_color_image->SetImage(m_logicalDevice, image_create_info);

		const auto memory_requirements = swap_chain_color_image->GetMemoryRequirements(m_logicalDevice);
		const auto memory_type_id = FindMemoryType(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
		vk::MemoryAllocateInfo allocate_info(memory_requirements.size, memory_type_id);
		swap_chain_color_image->SetMemory(m_logicalDevice, allocate_info);

		swap_chain_color_image->BindMemory(m_logicalDevice);
		swap_chain_color_image->SetImageView(m_logicalDevice,
			hephics_helper::simple_create_info::get_swap_chain_color_image_view_info(color_format));
	}

	{
		auto& swap_chain_depth_image = m_ptrSwapChain->GetDepthImage();

		vk::ImageCreateInfo image_create_info({}, vk::ImageType::e2D, depth_format,
			vk::Extent3D(m_ptrSwapChain->GetExtent2D(), 1U), 1, 1, image_sample_count, vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::SharingMode::eExclusive, queue_family_array);
		swap_chain_depth_image->SetImage(m_logicalDevice, image_create_info);

		const auto memory_requirements = swap_chain_depth_image->GetMemoryRequirements(m_logicalDevice);
		const auto memory_type_id = FindMemoryType(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
		vk::MemoryAllocateInfo allocate_info(memory_requirements.size, memory_type_id);
		swap_chain_depth_image->SetMemory(m_logicalDevice, allocate_info);

		swap_chain_depth_image->BindMemory(m_logicalDevice);
		swap_chain_depth_image->SetImageView(m_logicalDevice,
			hephics_helper::simple_create_info::get_swap_chain_depth_image_view_info(depth_format));
	}

	const auto& swap_chain_extent = m_ptrSwapChain->GetExtent2D();
	vk::FramebufferCreateInfo framebuffer_info({}, m_ptrSwapChain->GetRenderPass().get(), {},
		swap_chain_extent.width, swap_chain_extent.height, 1);
	m_ptrSwapChain->SetFramebuffers(m_logicalDevice, framebuffer_info);
}

void hephics::VkInstance::SetSwapChainSyncObjects()
{
	m_ptrSwapChain->SetSyncObjects(m_logicalDevice, hephics::BUFFERING_FRAME_NUM);
}

void hephics::VkInstance::SetCommandPools()
{
	{
		vk::CommandPoolCreateInfo create_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			m_queueFamilyIndices.graphics_and_compute_family.value());

		const auto command_pool_size = m_ptrSwapChain->GetFramebuffers().size();
		std::vector<std::unordered_map<std::string, vk::UniqueCommandPool>> command_pools(command_pool_size);
		for (size_t idx = 0; idx < command_pool_size; idx++)
			for (const auto& purpose : GPUHandler::GetGraphicPurpose())
				command_pools.at(idx).emplace(purpose, m_logicalDevice->createCommandPoolUnique(create_info));

		m_commandPoolsDictionary.emplace(vk::QueueFlagBits::eGraphics, std::move(command_pools));
	}

	{
		vk::CommandPoolCreateInfo create_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			m_queueFamilyIndices.graphics_and_compute_family.value());

		const auto command_pool_size = BUFFERING_FRAME_NUM;
		std::vector<std::unordered_map<std::string, vk::UniqueCommandPool>> command_pools(command_pool_size);
		for (size_t idx = 0; idx < command_pool_size; idx++)
			for (const auto& purpose : GPUHandler::GetComputePurpose())
				command_pools.at(idx).emplace(purpose, m_logicalDevice->createCommandPoolUnique(create_info));

		m_commandPoolsDictionary.emplace(vk::QueueFlagBits::eCompute, std::move(command_pools));
	}
}

void hephics::VkInstance::SetCommandBuffers()
{
	{
		const auto& command_pools = m_commandPoolsDictionary.at(vk::QueueFlagBits::eGraphics);
		decltype(m_graphicCommandBuffers) command_buffers(command_pools.size());

		size_t buffer_idx = 0U;
		for (const auto& command_pool : command_pools)
		{
			for (const auto& purpose : GPUHandler::GetGraphicPurpose())
			{
				vk::CommandBufferAllocateInfo alloc_info(command_pool.at(purpose).get(), vk::CommandBufferLevel::ePrimary, 1);
				auto unique_command_buffers = m_logicalDevice->allocateCommandBuffersUnique(alloc_info);

				vk_interface::component::CommandBuffer new_command_buffer;
				new_command_buffer.SetCommandBuffer(std::move(unique_command_buffers));
				command_buffers.at(buffer_idx).emplace(
					purpose, std::make_shared<vk_interface::component::CommandBuffer>(std::move(new_command_buffer)));
			}
			buffer_idx++;
		}

		m_graphicCommandBuffers = std::move(command_buffers);
	}

	{
		const auto& command_pools = m_commandPoolsDictionary.at(vk::QueueFlagBits::eCompute);
		decltype(m_computeCommandBuffers) command_buffers(command_pools.size());

		size_t buffer_idx = 0U;
		for (const auto& command_pool : command_pools)
		{
			for (const auto& purpose : GPUHandler::GetComputePurpose())
			{
				vk::CommandBufferAllocateInfo alloc_info(command_pool.at(purpose).get(), vk::CommandBufferLevel::ePrimary, 1);
				auto unique_command_buffers = m_logicalDevice->allocateCommandBuffersUnique(alloc_info);

				vk_interface::component::CommandBuffer new_command_buffer;
				new_command_buffer.SetCommandBuffer(std::move(unique_command_buffers));
				command_buffers.at(buffer_idx).emplace(
					purpose, std::make_shared<vk_interface::component::CommandBuffer>(std::move(new_command_buffer)));
			}
			buffer_idx++;
		}

		m_computeCommandBuffers = std::move(command_buffers);
	}
}

hephics::VkInstance::VkInstance()
	: Instance()
{
	SetInstance(hephics_helper::simple_create_info::get_application_info(
		window::Manager::GetWindow()->GetWindowTitle()));

	SetWindowSurface();

	SetPhysicalDevice();
	SetLogicalDeviceAndQueue();

	SetSwapChain();
	SetSwapChainImageViews();
	SetSwapChainRenderPass();
	SetSwapChainFramebuffers();
	SetSwapChainSyncObjects();

	SetCommandPools();
	SetCommandBuffers();
}

void hephics::VkInstance::ResetSwapChain(::GLFWwindow* const ptr_window)
{
	window::WindowInfo create_info;
	::glfwGetFramebufferSize(ptr_window, &(create_info.width), &(create_info.height));
	while (create_info.width == 0 || create_info.height == 0);
	{
		::glfwGetFramebufferSize(ptr_window, &(create_info.width), &(create_info.height));
		::glfwWaitEvents();
	}

	m_logicalDevice->waitIdle();
	m_ptrSwapChain->Clear(m_logicalDevice);

	SetSwapChain();
	SetSwapChainImageViews();
	SetSwapChainFramebuffers();
	SetSwapChainSyncObjects();
}

void hephics::VkInstance::SubmitCopyGraphicResource(const vk::SubmitInfo& submit_info)
{
	if (!m_queuesDictionary.contains(vk::QueueFlagBits::eGraphics))
		throw std::runtime_error("queue: not found");

	m_queuesDictionary.at(vk::QueueFlagBits::eGraphics).at("graphics").submit(submit_info, nullptr);
	m_logicalDevice->waitIdle();
}

void hephics::VkInstance::SubmitRenderingCommand(const vk::SubmitInfo& submit_info)
{
	if (!m_queuesDictionary.contains(vk::QueueFlagBits::eGraphics))
		throw std::runtime_error("queue: not found");

	const auto& current_fence = m_ptrSwapChain->GetCurrentSwapFence()->GetFence();
	m_queuesDictionary.at(vk::QueueFlagBits::eGraphics).at("graphics").submit(submit_info, current_fence.get());
}

void hephics::VkInstance::PresentFrame(const vk::PresentInfoKHR& present_info)
{
	if (!m_queuesDictionary.contains(vk::QueueFlagBits::eGraphics))
		throw std::runtime_error("queue: not found");

	vk::resultCheck(m_queuesDictionary.at(vk::QueueFlagBits::eGraphics).at("present").presentKHR(present_info),
		"failed to present");
}

void hephics::VkInstance::SubmitComputingCommand(const vk::SubmitInfo& submit_info)
{
	if (!m_queuesDictionary.contains(vk::QueueFlagBits::eCompute))
		throw std::runtime_error("queue: not found");

	const auto& current_fence = m_ptrComputingSyncObject->GetCurrentFence()->GetFence();
	m_queuesDictionary.at(vk::QueueFlagBits::eCompute).at("compute").submit(submit_info, current_fence.get());
}

vk::Format hephics::VkInstance::FindSupportedFormat(const std::vector<vk::Format>& candidates,
	const vk::ImageTiling& tilling, const vk::FormatFeatureFlags& features) const
{
	return hephics_helper::vk_init::find_supported_format(
		m_physicalDevice, candidates, tilling, features);
}

vk::Format hephics::VkInstance::FindDepthFormat() const
{
	return hephics_helper::vk_init::find_depth_format(m_physicalDevice);
}

uint32_t hephics::VkInstance::FindMemoryType(const uint32_t& memory_type_filter,
	const vk::MemoryPropertyFlags& memory_prop_flags) const
{
	return hephics_helper::vk_init::find_memory_type(
		m_physicalDevice, memory_type_filter, memory_prop_flags);
}

vk::SampleCountFlagBits hephics::VkInstance::GetMultiSampleCount() const
{
	return hephics_helper::vk_init::get_multi_sample_count(m_physicalDevice);
}

std::shared_ptr<vk_interface::component::CommandBuffer>& hephics::VkInstance::GetGraphicCommandBuffer(const std::string& purpose)
{
	const auto& next_image_id = m_ptrSwapChain->GetNextImageId();
	return m_graphicCommandBuffers.at(next_image_id).at(purpose);
}

std::shared_ptr<vk_interface::component::CommandBuffer>& hephics::VkInstance::GetComputeCommandBuffer(const std::string& purpose)
{
	const auto& current_frame_id = m_ptrComputingSyncObject->GetCurrentFrameId();
	return m_computeCommandBuffers.at(current_frame_id).at(purpose);
}