#include "../HephicsHelper.hpp"

void hephics_helper::GPUBuffer::Initialize(
	const std::shared_ptr<vk_interface::Instance>& gpu_instance,
	const size_t& buffer_size, const vk::BufferUsageFlags& usage_flags)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();

	const auto& queue_family_array =
		vk_init::find_queue_families(physical_device, window_surface).get_families_array();

	SetBuffer(logical_device,
		simple_create_info::get_gpu_buffer_info(gpu_instance, buffer_size, usage_flags));

	const auto& memory_requirements = GetMemoryRequirements(logical_device);
	const auto& memory_type_idx = vk_init::find_memory_type(
		physical_device, memory_requirements.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	vk::MemoryAllocateInfo allocate_info(memory_requirements.size, memory_type_idx);
	SetMemory(logical_device, allocate_info);

	BindMemory(logical_device);
}

void hephics_helper::UniformBuffer::Initialize(
	const std::shared_ptr<vk_interface::Instance>& gpu_instance, const size_t& buffer_size)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();

	const auto& queue_family_array =
		vk_init::find_queue_families(physical_device, window_surface).get_families_array();

	vk::BufferCreateInfo uniform_buffer_info({}, buffer_size,
		vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive, queue_family_array);
	SetBuffer(logical_device, uniform_buffer_info);

	const auto& memory_requirements = GetMemoryRequirements(logical_device);
	const auto& memory_type_idx = vk_init::find_memory_type(
		physical_device, memory_requirements.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	vk::MemoryAllocateInfo allocate_info(memory_requirements.size, memory_type_idx);
	SetMemory(logical_device, allocate_info);

	BindMemory(logical_device);
}

void hephics_helper::StagingBuffer::Initialize(
	const std::shared_ptr<vk_interface::Instance>& gpu_instance, const size_t& buffer_size)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();

	const auto& queue_family_array =
		vk_init::find_queue_families(physical_device, window_surface).get_families_array();

	vk::BufferCreateInfo staging_buffer_info({}, buffer_size,
		vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, queue_family_array);
	SetBuffer(logical_device, staging_buffer_info);

	const auto& memory_requirements = GetMemoryRequirements(logical_device);
	const auto& memory_type_idx = vk_init::find_memory_type(
		physical_device, memory_requirements.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	vk::MemoryAllocateInfo allocate_info(memory_requirements.size, memory_type_idx);
	SetMemory(logical_device, allocate_info);

	BindMemory(logical_device);
}

#ifdef _DEBUG
vk::DebugUtilsMessageSeverityFlagsEXT hephics_helper::simple_create_info::get_severity_flags()
{
	return vk::DebugUtilsMessageSeverityFlagsEXT(
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
}

vk::DebugUtilsMessageTypeFlagsEXT hephics_helper::simple_create_info::get_message_type_flags()
{
	return vk::DebugUtilsMessageTypeFlagsEXT(
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
}
#endif

vk::ApplicationInfo hephics_helper::simple_create_info::get_application_info(
	const std::string& window_title, const std::string& engine_name)
{
	return vk::ApplicationInfo(
		window_title.c_str(),
		VK_MAKE_VERSION(1, 0, 0),
		engine_name.c_str(),
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_3);
}

vk::ImageViewCreateInfo hephics_helper::simple_create_info::get_swap_chain_image_view_info()
{
	return vk::ImageViewCreateInfo(
		{}, {}, vk::ImageViewType::e2D, {},
		vk::ComponentMapping(
			vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
}

vk::ImageViewCreateInfo hephics_helper::simple_create_info::get_swap_chain_depth_image_view_info(const vk::Format& format)
{
	return 	vk::ImageViewCreateInfo({}, {}, vk::ImageViewType::e2D, format,
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
}

vk::ImageViewCreateInfo hephics_helper::simple_create_info::get_swap_chain_color_image_view_info(const vk::Format& format)
{
	return vk::ImageViewCreateInfo({}, {}, vk::ImageViewType::e2D, format,
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
}

std::vector<vk::AttachmentDescription> hephics_helper::simple_create_info::get_renderpass_attachment_descriptions(
	const vk::SampleCountFlagBits& sample_count, const vk::Format& color_format, const vk::Format& depth_format)
{
	vk::AttachmentDescription color_attachment({}, color_format,
		sample_count, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentDescription depth_attachment({}, depth_format,
		sample_count, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::AttachmentDescription color_resolve_attachment({}, color_format,
		vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

	return std::vector{ color_attachment, depth_attachment, color_resolve_attachment };
}

vk::SubpassDependency hephics_helper::simple_create_info::get_renderpass_dependency()
{
	return vk::SubpassDependency(
		0, 0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
		vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
		vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
		vk::DependencyFlagBits::eByRegion
	);
}

vk::BufferCreateInfo hephics_helper::simple_create_info::get_gpu_buffer_info(
	const std::shared_ptr<vk_interface::Instance>& gpu_instance,
	const size_t& buffer_size, const vk::BufferUsageFlags& usage_flags)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& queue_family_array =
		vk_init::find_queue_families(physical_device, window_surface).get_families_array();

	return vk::BufferCreateInfo(
		{}, buffer_size, usage_flags | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eExclusive, queue_family_array
	);
}

vk::ImageCreateInfo hephics_helper::simple_create_info::get_texture_image_info(
	const std::shared_ptr<vk_interface::Instance>& gpu_instance, const vk::Extent2D& extent)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& queue_family_array =
		vk_init::find_queue_families(physical_device, window_surface).get_families_array();

	// eTransferSrc: In order to send image bilt, purpose of mipmapping
	return vk::ImageCreateInfo(
		{}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
		vk::Extent3D(extent, 1U), 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive, queue_family_array
	);
}

vk::ImageViewCreateInfo hephics_helper::simple_create_info::get_texture_image_view_info(const vk::UniqueImage& image)
{
	return vk::ImageViewCreateInfo(
		{}, image.get(), vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Srgb,
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	);
}

vk::SamplerCreateInfo hephics_helper::simple_create_info::get_texture_sampler_info(
	const std::shared_ptr<vk_interface::Instance>& gpu_instance)
{
	const auto& physical_device_props = gpu_instance->GetPhysicalDevice().getProperties();

	return vk::SamplerCreateInfo(
		{}, vk::Filter::eLinear, vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
		0.0f, VK_TRUE, physical_device_props.limits.maxSamplerAnisotropy, VK_FALSE,
		vk::CompareOp::eAlways, 0.0f, 0.0f, vk::BorderColor::eIntOpaqueBlack, VK_FALSE
	);
}